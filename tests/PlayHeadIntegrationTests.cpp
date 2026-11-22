// SPDX-License-Identifier: AGPL-3.0-only

#include "model/CueState.h"
#include "model/LyricCue.h"
#include "plugin/PluginProcessor.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace
{
void require(bool condition, const char *message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class TestPlayHead final : public juce::AudioPlayHead
{
  public:
    juce::Optional<PositionInfo> getPosition() const override
    {
        return position_;
    }

    void setPosition(juce::Optional<PositionInfo> position)
    {
        position_ = std::move(position);
    }

  private:
    juce::Optional<PositionInfo> position_;
};

std::unique_ptr<const showlyrics::CueState> makeState(double firstStart, double secondStart)
{
    std::vector<showlyrics::LyricCue> cues;
    cues.emplace_back("123e4567-e89b-42d3-a456-426614174000", firstStart, firstStart + 4.0, "First", "First lyric", 1);
    cues.emplace_back("123e4567-e89b-42d3-a456-426614174001", secondStart, secondStart + 4.0, "Second", "Second lyric",
                      0);

    showlyrics::CueStateValidationResult validation;
    auto state = showlyrics::CueState::create(std::move(cues), validation);
    require(state.has_value(), "test CueState must be valid");
    return std::make_unique<const showlyrics::CueState>(std::move(*state));
}

void publishState(ShowLyricsAudioProcessor &processor, std::unique_ptr<const showlyrics::CueState> state,
                  std::uint64_t revision)
{
    require(processor.publishCueState(state, revision) == showlyrics::CueGenerationPublishResult::published,
            "CueState publication must succeed");
    require(state == nullptr, "successful publication must transfer CueState ownership");
}

showlyrics::TransportSnapshotReadResult processAndConsume(ShowLyricsAudioProcessor &processor)
{
    juce::AudioBuffer<float> buffer(2, 16);
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            buffer.setSample(channel, sample, static_cast<float>(channel * 100 + sample) / 100.0F);
    }

    std::vector<float> original;
    original.reserve(static_cast<std::size_t>(buffer.getNumChannels() * buffer.getNumSamples()));
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            original.push_back(buffer.getSample(channel, sample));
    }

    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);

    std::size_t index = 0;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            require(buffer.getSample(channel, sample) == original[index], "processBlock must preserve audio samples");
            ++index;
        }
    }

    return processor.consumeLatestTransportSnapshot();
}

juce::AudioPlayHead::PositionInfo makePlayingPosition(double ppqPosition)
{
    juce::AudioPlayHead::PositionInfo position;
    position.setIsPlaying(true);
    position.setPpqPosition(ppqPosition);
    return position;
}

void testCompleteAndPartialHostPositions()
{
    ShowLyricsAudioProcessor processor;
    TestPlayHead playHead;
    processor.setPlayHead(&playHead);
    processor.prepareToPlay(48'000.0, 512);
    publishState(processor, makeState(0.0, 8.0), 1);

    auto position = makePlayingPosition(1.5);
    position.setPpqPositionOfLastBarStart(0.0);
    position.setBpm(123.0);
    position.setTimeSignature(juce::AudioPlayHead::TimeSignature{7, 8});
    position.setIsRecording(true);
    position.setIsLooping(true);
    position.setLoopPoints(juce::AudioPlayHead::LoopPoints{0.0, 16.0});
    playHead.setPosition(position);

    const auto complete = processAndConsume(processor);
    require(complete.canDisplayActiveCue(), "a complete playing position must be displayable");
    require(complete.snapshot.cueRevision == 1 && complete.snapshot.activeCueIndex == 0,
            "active cue and revision must come from the same generation");
    require(complete.snapshot.ppqPosition == 1.5 && complete.snapshot.barStartQuarterNotes == 0.0 &&
                complete.snapshot.tempoBpm == 123.0,
            "valid musical host values must be captured");
    require(complete.snapshot.timeSignatureNumerator == 7 && complete.snapshot.timeSignatureDenominator == 8,
            "valid time signature must be captured");
    require(complete.snapshot.cycleStartQuarterNotes == 0.0 && complete.snapshot.cycleEndQuarterNotes == 16.0,
            "valid cycle range must be captured");
    require(complete.snapshot.is(showlyrics::TransportSnapshotState::playing) &&
                complete.snapshot.is(showlyrics::TransportSnapshotState::recording) &&
                complete.snapshot.is(showlyrics::TransportSnapshotState::cycling),
            "transport flags must be captured");

    playHead.setPosition(makePlayingPosition(8.5));
    const auto partial = processAndConsume(processor);
    require(partial.canDisplayActiveCue() && partial.snapshot.activeCueIndex == 1,
            "missing optional metadata must not prevent cue selection");
    require(!partial.snapshot.has(showlyrics::TransportSnapshotField::tempo) &&
                !partial.snapshot.has(showlyrics::TransportSnapshotField::timeSignature) &&
                !partial.snapshot.has(showlyrics::TransportSnapshotField::cycleRange),
            "missing optional metadata must remain distinguishable");
}

void testInvalidAndMissingHostPositions()
{
    ShowLyricsAudioProcessor processor;
    TestPlayHead playHead;
    processor.setPlayHead(&playHead);
    processor.prepareToPlay(48'000.0, 512);
    publishState(processor, makeState(0.0, 8.0), 1);

    auto invalid = makePlayingPosition(std::numeric_limits<double>::quiet_NaN());
    invalid.setPpqPositionOfLastBarStart(std::numeric_limits<double>::infinity());
    invalid.setBpm(-1.0);
    invalid.setTimeSignature(juce::AudioPlayHead::TimeSignature{0, 4});
    invalid.setLoopPoints(juce::AudioPlayHead::LoopPoints{4.0, 4.0});
    playHead.setPosition(invalid);

    const auto invalidResult = processAndConsume(processor);
    require(!invalidResult.snapshot.has(showlyrics::TransportSnapshotField::ppqPosition) &&
                !invalidResult.snapshot.has(showlyrics::TransportSnapshotField::barStartPosition) &&
                !invalidResult.snapshot.has(showlyrics::TransportSnapshotField::tempo) &&
                !invalidResult.snapshot.has(showlyrics::TransportSnapshotField::timeSignature) &&
                !invalidResult.snapshot.has(showlyrics::TransportSnapshotField::cycleRange),
            "invalid numeric host values must be rejected independently");
    require(!invalidResult.snapshot.has(showlyrics::TransportSnapshotField::activeCue),
            "invalid PPQ must prevent cue selection");

    playHead.setPosition({});
    const auto missingResult = processAndConsume(processor);
    require(!missingResult.snapshot.has(showlyrics::TransportSnapshotField::transportState) &&
                !missingResult.snapshot.has(showlyrics::TransportSnapshotField::activeCue),
            "missing PositionInfo must not imply transport state or an active cue");
}

void testPlayStopSeekAndCycleWrap()
{
    ShowLyricsAudioProcessor processor;
    TestPlayHead playHead;
    processor.setPlayHead(&playHead);
    processor.prepareToPlay(48'000.0, 512);
    publishState(processor, makeState(0.0, 8.0), 1);

    playHead.setPosition(makePlayingPosition(0.0));
    require(processAndConsume(processor).snapshot.activeCueIndex == 0, "cue start must be inclusive");

    playHead.setPosition(makePlayingPosition(4.0));
    require(!processAndConsume(processor).snapshot.has(showlyrics::TransportSnapshotField::activeCue),
            "cue end must be exclusive");

    playHead.setPosition(makePlayingPosition(9.0));
    require(processAndConsume(processor).snapshot.activeCueIndex == 1, "forward seek must select from host PPQ");

    playHead.setPosition(makePlayingPosition(1.0));
    require(processAndConsume(processor).snapshot.activeCueIndex == 0,
            "backward seek must select directly from host PPQ");

    auto cycleEnd = makePlayingPosition(11.5);
    cycleEnd.setIsLooping(true);
    cycleEnd.setLoopPoints(juce::AudioPlayHead::LoopPoints{8.0, 12.0});
    playHead.setPosition(cycleEnd);
    require(processAndConsume(processor).snapshot.activeCueIndex == 1, "cycle end block must select by host PPQ");

    auto cycleStart = makePlayingPosition(8.0);
    cycleStart.setIsLooping(true);
    cycleStart.setLoopPoints(juce::AudioPlayHead::LoopPoints{8.0, 12.0});
    playHead.setPosition(cycleStart);
    require(processAndConsume(processor).snapshot.activeCueIndex == 1,
            "cycle wrap must require no inferred position progression");

    auto stopped = makePlayingPosition(1.0);
    stopped.setIsPlaying(false);
    playHead.setPosition(stopped);
    const auto stoppedResult = processAndConsume(processor);
    require(stoppedResult.snapshot.has(showlyrics::TransportSnapshotField::transportState) &&
                !stoppedResult.snapshot.is(showlyrics::TransportSnapshotState::playing) &&
                !stoppedResult.snapshot.has(showlyrics::TransportSnapshotField::activeCue),
            "an observed stop must clear the active cue");
}

void testGenerationSwitchAndMailboxOverflow()
{
    ShowLyricsAudioProcessor processor;
    TestPlayHead playHead;
    processor.setPlayHead(&playHead);
    processor.prepareToPlay(48'000.0, 512);

    publishState(processor, makeState(0.0, 8.0), 1);
    playHead.setPosition(makePlayingPosition(1.0));
    const auto first = processAndConsume(processor);
    require(first.snapshot.cueRevision == 1 && first.snapshot.activeCueIndex == 0,
            "first generation must be reported consistently");
    bool visitedFirstCue = false;
    require(processor.visitCueState(first.snapshot.cueRevision,
                                    [&](const showlyrics::CueState &state) {
                                        visitedFirstCue =
                                            state.getCues().at(first.snapshot.activeCueIndex).getLyrics() ==
                                            "First lyric";
                                    }) &&
                visitedFirstCue,
            "snapshot revision and cue index must resolve under bounded non-audio ownership");

    publishState(processor, makeState(20.0, 28.0), 2);
    playHead.setPosition(makePlayingPosition(20.0));
    const auto second = processAndConsume(processor);
    require(second.snapshot.cueRevision == 2 && second.snapshot.activeCueIndex == 0,
            "new revision and its cue index must activate together");

    publishState(processor, makeState(40.0, 48.0), 3);
    require(!processor.visitCueState(1, [](const showlyrics::CueState &) {}),
            "a reclaimed generation must fail lookup instead of exposing stale cue text");

    playHead.setPosition(makePlayingPosition(20.0));
    for (std::size_t index = 0; index < showlyrics::TransportSnapshotMailbox::capacity; ++index)
    {
        juce::AudioBuffer<float> buffer(2, 1);
        juce::MidiBuffer midi;
        processor.processBlock(buffer, midi);
    }

    auto stopped = makePlayingPosition(20.0);
    stopped.setIsPlaying(false);
    playHead.setPosition(stopped);
    {
        juce::AudioBuffer<float> buffer(2, 1);
        juce::MidiBuffer midi;
        processor.processBlock(buffer, midi);
    }

    const auto overflowed = processor.consumeLatestTransportSnapshot();
    require(overflowed.overflowed && !overflowed.canDisplayActiveCue(),
            "mailbox overflow must make queued cue state explicitly unsafe to display");

    const auto observedStop = processAndConsume(processor);
    require(!observedStop.overflowed && observedStop.hasSnapshot &&
                !observedStop.snapshot.has(showlyrics::TransportSnapshotField::activeCue),
            "a later accepted snapshot must restore a known stopped state");

    processor.releaseResources();
    const auto released = processor.consumeLatestTransportSnapshot();
    require(!released.audioProcessingAvailable && !released.canDisplayActiveCue(),
            "released audio processing must invalidate cue display eligibility");

    playHead.setPosition(makePlayingPosition(20.0));
    processor.prepareToPlay(48'000.0, 512);
    {
        juce::AudioBuffer<float> buffer(2, 1);
        juce::MidiBuffer midi;
        processor.processBlock(buffer, midi);
    }
    processor.releaseResources();
    processor.prepareToPlay(48'000.0, 512);
    const auto restarted = processor.consumeLatestTransportSnapshot();
    require(restarted.audioProcessingAvailable && !restarted.hasSnapshot,
            "prepareToPlay must discard snapshots from the previous audio lifecycle");
}
} // namespace

int main()
{
    testCompleteAndPartialHostPositions();
    testInvalidAndMissingHostPositions();
    testPlayStopSeekAndCycleWrap();
    testGenerationSwitchAndMailboxOverflow();

    std::cout << "ShowLyricsPlayHeadIntegrationTests: PASS\n";
    return EXIT_SUCCESS;
}
