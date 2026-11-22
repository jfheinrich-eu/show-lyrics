// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "CuePlaybackIndex.h"
#include "CueState.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

namespace showlyrics
{
enum class CueGenerationPublishResult
{
    published,
    invalidState,
    invalidRevision,
    staleRevision,
    slotsFull
};

struct CueGenerationAudioView final
{
    const CuePlaybackIndex *playbackIndex{nullptr};
    std::uint64_t revision{0};

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return playbackIndex != nullptr;
    }
};

// Single-producer/single-consumer exchange. The control thread owns publication,
// CueState access, reclamation, and destruction. The audio thread may only call
// activateLatestForAudio() and currentAudioView(). Slot ownership follows
// free -> writing -> ready -> active/retired -> reclaiming -> free. Only the
// control thread enters writing/reclaiming; only the audio thread enters active
// or retires ready/active slots. Destruction must occur after audio processing
// has stopped.
class CueGenerationExchange final
{
  public:
    static constexpr std::size_t slotCount = 3;

    CueGenerationExchange() = default;
    ~CueGenerationExchange() = default;

    CueGenerationExchange(const CueGenerationExchange &) = delete;
    CueGenerationExchange &operator=(const CueGenerationExchange &) = delete;
    CueGenerationExchange(CueGenerationExchange &&) = delete;
    CueGenerationExchange &operator=(CueGenerationExchange &&) = delete;

    // Revisions are non-zero, strictly increasing, and never wrap. On success,
    // ownership moves into a slot. On failure, cueState remains owned by the
    // caller and the same revision may be retried after slots are reclaimed.
    [[nodiscard]] CueGenerationPublishResult tryPublish(std::unique_ptr<const CueState> &cueState,
                                                        std::uint64_t revision);

    // Control-thread only. Returned pointers remain valid until the control thread
    // next calls reclaimRetired().
    [[nodiscard]] const CueState *findCueState(std::uint64_t revision) const noexcept;
    [[nodiscard]] std::size_t reclaimRetired() noexcept;

    // Audio-thread only. The returned view remains valid until the next successful
    // activation by the audio thread. Neither function touches CueState ownership.
    [[nodiscard]] CueGenerationAudioView activateLatestForAudio() noexcept;
    [[nodiscard]] CueGenerationAudioView currentAudioView() const noexcept;

  private:
    enum class SlotState : std::uint8_t
    {
        free,
        writing,
        ready,
        active,
        retired,
        reclaiming
    };

    struct Slot final
    {
        std::atomic<std::uint8_t> state{static_cast<std::uint8_t>(SlotState::free)};
        std::unique_ptr<const CueState> cueState;
        std::optional<CuePlaybackIndex> playbackIndex;
        std::uint64_t revision{0};
    };

    [[nodiscard]] static std::uint8_t stateValue(SlotState state) noexcept;
    void retireReadyAtOrBelow(std::uint64_t revision) noexcept;

    static_assert(std::atomic<std::uint8_t>::is_always_lock_free);

    std::array<Slot, slotCount> slots_{};
    std::uint64_t lastPublishedRevision_{0}; // Control-thread only.
    std::size_t activeSlot_{slotCount};      // Audio-thread only.
    std::uint64_t activeRevision_{0};        // Audio-thread only.
};
} // namespace showlyrics
