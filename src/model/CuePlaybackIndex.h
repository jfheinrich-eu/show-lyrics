// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "CueStateLimits.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>

namespace showlyrics
{
class CueState;

class CuePlaybackIndex final
{
  public:
    // Builds the complete index and must only be called outside the audio callback.
    explicit CuePlaybackIndex(const CueState &state);

    // Returns the index of the matching cue in the source CueState.
    [[nodiscard]] std::optional<std::uint16_t> findCueIndex(double positionQuarterNotes) const noexcept;
    [[nodiscard]] std::size_t getCueCount() const noexcept;

  private:
    static_assert(limits::maximumCueCount <= static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()));

    std::array<double, limits::maximumCueCount> startPositions_{};
    std::array<double, limits::maximumCueCount> endPositions_{};
    std::array<std::uint16_t, limits::maximumCueCount> cueIndices_{};
    std::uint16_t cueCount_{0};
};

static_assert(sizeof(CuePlaybackIndex) <= 20U * 1024U);
} // namespace showlyrics
