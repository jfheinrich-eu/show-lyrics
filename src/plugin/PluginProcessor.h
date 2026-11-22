// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "model/CueGenerationExchange.h"
#include "model/TransportSnapshotMailbox.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>

namespace showlyrics
{
struct TransportSnapshotReadResult final
{
    TransportSnapshot snapshot{};
    bool hasSnapshot{false};
    bool overflowed{false};
    bool audioProcessingAvailable{false};

    [[nodiscard]] bool canDisplayActiveCue() const noexcept
    {
        return hasSnapshot && !overflowed && audioProcessingAvailable &&
               snapshot.has(TransportSnapshotField::activeCue);
    }
};
} // namespace showlyrics

class ShowLyricsAudioProcessor final : public juce::AudioProcessor
{
  public:
    ShowLyricsAudioProcessor();

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    [[nodiscard]] bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
    void processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) override;

    [[nodiscard]] juce::AudioProcessorEditor *createEditor() override;
    [[nodiscard]] bool hasEditor() const override;

    [[nodiscard]] const juce::String getName() const override;
    [[nodiscard]] bool acceptsMidi() const override;
    [[nodiscard]] bool producesMidi() const override;
    [[nodiscard]] bool isMidiEffect() const override;
    [[nodiscard]] double getTailLengthSeconds() const override;

    [[nodiscard]] int getNumPrograms() override;
    [[nodiscard]] int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    [[nodiscard]] const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;

    void getStateInformation(juce::MemoryBlock &destinationData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    // Safe for callers outside the audio thread. Publication callers are
    // serialised because JUCE does not guarantee that state restoration runs
    // on the message thread. On failure, cueState remains caller-owned.
    [[nodiscard]] showlyrics::CueGenerationPublishResult publishCueState(
        std::unique_ptr<const showlyrics::CueState> &cueState, std::uint64_t revision);

    // Provides bounded-lifetime access to cue text on a non-audio thread. The
    // visitor must not retain references or call publishCueState recursively.
    // A false result means that the snapshot's generation was already retired.
    template <typename Visitor> [[nodiscard]] bool visitCueState(std::uint64_t revision, Visitor &&visitor)
    {
        const std::lock_guard<std::mutex> lock(cuePublicationMutex_);
        const auto *cueState = cueGenerations_.findCueState(revision);
        if (cueState == nullptr)
            return false;

        std::forward<Visitor>(visitor)(*cueState);
        return true;
    }

    // Safe for non-audio callers. Multiple potential editor/lifecycle callers
    // are serialised to preserve the mailbox's single-consumer contract. If
    // overflowed is true, the returned snapshot must not be trusted for cue
    // display until a later non-overflowed snapshot is consumed. If a host
    // stops calling processBlock without calling releaseResources, this layer
    // cannot observe that stop and deliberately does not synthesize one. The
    // future editor must treat snapshot age conservatively.
    [[nodiscard]] showlyrics::TransportSnapshotReadResult consumeLatestTransportSnapshot();

  private:
    showlyrics::CueGenerationExchange cueGenerations_;
    showlyrics::TransportSnapshotMailbox transportMailbox_;
    std::mutex cuePublicationMutex_;
    std::mutex snapshotConsumptionMutex_;
    std::atomic<bool> audioProcessingAvailable_{false};

    static_assert(std::atomic<bool>::is_always_lock_free);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShowLyricsAudioProcessor)
};
