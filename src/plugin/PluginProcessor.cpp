// SPDX-License-Identifier: AGPL-3.0-only

#include "PluginProcessor.h"

#include "HostPositionSnapshot.h"
#include "PluginEditor.h"

ShowLyricsAudioProcessor::ShowLyricsAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void ShowLyricsAudioProcessor::prepareToPlay(double, int)
{
    // JUCE calls this outside processBlock while audio processing is stopped.
    // Serialising with external consumers prevents a queued snapshot from a
    // previous audio lifecycle becoming displayable after restart.
    const std::lock_guard<std::mutex> lock(snapshotConsumptionMutex_);
    static_cast<void>(transportMailbox_.consumeOverflowFlag());
    showlyrics::TransportSnapshot discardedSnapshot;
    for (std::size_t count = 0; count < showlyrics::TransportSnapshotMailbox::capacity; ++count)
    {
        if (!transportMailbox_.tryConsume(discardedSnapshot))
            break;
    }
    audioProcessingAvailable_.store(true, std::memory_order_release);
}

void ShowLyricsAudioProcessor::releaseResources()
{
    audioProcessingAvailable_.store(false, std::memory_order_release);
}

bool ShowLyricsAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    const auto supportedInput = input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo();
    return supportedInput && input == output;
}

void ShowLyricsAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &)
{
    juce::ScopedNoDenormals noDenormals;

    const auto generation = cueGenerations_.activateLatestForAudio();
    const auto *hostPlayHead = getPlayHead();
    const auto position =
        hostPlayHead != nullptr ? hostPlayHead->getPosition() : juce::Optional<juce::AudioPlayHead::PositionInfo>{};
    const auto snapshot = showlyrics::makeTransportSnapshot(position ? &*position : nullptr, generation);
    // A full mailbox latches an overflow flag inside tryPublish(). The audio
    // thread never retries; the consumer must clear any displayed cue and wait
    // for a later accepted snapshot.
    static_cast<void>(transportMailbox_.tryPublish(snapshot));

    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());
}

showlyrics::CueGenerationPublishResult ShowLyricsAudioProcessor::publishCueState(
    std::unique_ptr<const showlyrics::CueState> &cueState, std::uint64_t revision)
{
    const std::lock_guard<std::mutex> lock(cuePublicationMutex_);
    static_cast<void>(cueGenerations_.reclaimRetired());
    return cueGenerations_.tryPublish(cueState, revision);
}

showlyrics::TransportSnapshotReadResult ShowLyricsAudioProcessor::consumeLatestTransportSnapshot()
{
    const std::lock_guard<std::mutex> lock(snapshotConsumptionMutex_);

    showlyrics::TransportSnapshotReadResult result;
    const auto overflowedBeforeDrain = transportMailbox_.consumeOverflowFlag();

    showlyrics::TransportSnapshot snapshot;
    for (std::size_t count = 0; count < showlyrics::TransportSnapshotMailbox::capacity; ++count)
    {
        if (!transportMailbox_.tryConsume(snapshot))
            break;

        result.snapshot = snapshot;
        result.hasSnapshot = true;
    }

    result.overflowed = overflowedBeforeDrain || transportMailbox_.consumeOverflowFlag();
    result.audioProcessingAvailable = audioProcessingAvailable_.load(std::memory_order_acquire);
    return result;
}

juce::AudioProcessorEditor *ShowLyricsAudioProcessor::createEditor()
{
    return new ShowLyricsAudioProcessorEditor(*this);
}

bool ShowLyricsAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String ShowLyricsAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ShowLyricsAudioProcessor::acceptsMidi() const
{
    return false;
}

bool ShowLyricsAudioProcessor::producesMidi() const
{
    return false;
}

bool ShowLyricsAudioProcessor::isMidiEffect() const
{
    return false;
}

double ShowLyricsAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ShowLyricsAudioProcessor::getNumPrograms()
{
    return 1;
}

int ShowLyricsAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ShowLyricsAudioProcessor::setCurrentProgram(int)
{
}

const juce::String ShowLyricsAudioProcessor::getProgramName(int)
{
    return "Default";
}

void ShowLyricsAudioProcessor::changeProgramName(int, const juce::String &)
{
}

void ShowLyricsAudioProcessor::getStateInformation(juce::MemoryBlock &destinationData)
{
    const juce::ValueTree state("ShowLyricsState");
    juce::MemoryOutputStream stream(destinationData, false);
    state.writeToStream(stream);
}

void ShowLyricsAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
        return;

    const auto state = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
    if (!state.isValid() || !state.hasType("ShowLyricsState"))
        return;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new ShowLyricsAudioProcessor();
}
