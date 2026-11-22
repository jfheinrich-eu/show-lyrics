// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "TransportSnapshotReadResult.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace showlyrics
{
enum class LyricsDisplayMode
{
    awaitingSnapshot,
    activeCue,
    stopped,
    invalidHostPosition,
    noCue,
    mailboxOverflow,
    audioUnavailable,
    staleSnapshot,
    cueUnavailable
};

struct ActiveCueReference final
{
    std::uint64_t revision{0};
    std::uint16_t cueIndex{0};

    [[nodiscard]] bool operator==(const ActiveCueReference &other) const noexcept
    {
        return revision == other.revision && cueIndex == other.cueIndex;
    }

    [[nodiscard]] bool operator!=(const ActiveCueReference &other) const noexcept
    {
        return !(*this == other);
    }
};

// Message-thread state only. The timeout measures the age of received
// snapshots with a monotonic clock; it never advances musical position and a
// stale snapshot is deliberately distinct from an observed transport stop.
class LyricsDisplayModel final
{
  public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    static constexpr auto staleSnapshotTimeout = std::chrono::milliseconds{500};

    explicit LyricsDisplayModel(TimePoint openedAt) noexcept;

    template <typename Resolver>
    [[nodiscard]] bool update(const TransportSnapshotReadResult &result, TimePoint now, Resolver &&resolver)
    {
        const auto stateChanged = updateTransportState(result, now);
        if (!stateChanged)
            return false;

        if (mode_ != LyricsDisplayMode::activeCue)
        {
            lyrics_.clear();
            return true;
        }

        auto resolvedLyrics = std::forward<Resolver>(resolver)(*activeCue_);
        if (!resolvedLyrics)
        {
            mode_ = LyricsDisplayMode::cueUnavailable;
            lyrics_.clear();
            return true;
        }

        lyrics_ = std::move(*resolvedLyrics);
        return true;
    }

    [[nodiscard]] LyricsDisplayMode getMode() const noexcept;
    [[nodiscard]] const std::optional<ActiveCueReference> &getActiveCue() const noexcept;
    [[nodiscard]] const std::string &getLyrics() const noexcept;

  private:
    [[nodiscard]] bool updateTransportState(const TransportSnapshotReadResult &result, TimePoint now) noexcept;
    [[nodiscard]] bool setMode(LyricsDisplayMode mode) noexcept;
    [[nodiscard]] bool setActiveCue(ActiveCueReference cue) noexcept;
    [[nodiscard]] bool isStale(TimePoint now) const noexcept;

    LyricsDisplayMode mode_{LyricsDisplayMode::awaitingSnapshot};
    std::optional<ActiveCueReference> activeCue_;
    std::string lyrics_;
    TimePoint openedAt_;
    std::optional<TimePoint> lastSnapshotAt_;
    bool hasReceivedSnapshot_{false};
};
} // namespace showlyrics
