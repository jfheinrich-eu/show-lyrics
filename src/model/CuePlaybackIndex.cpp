// SPDX-License-Identifier: AGPL-3.0-only

#include "CuePlaybackIndex.h"

#include "CueState.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numeric>

namespace showlyrics
{
CuePlaybackIndex::CuePlaybackIndex(const CueState &state)
{
    const auto &cues = state.getCues();
    cueCount_ = static_cast<std::uint16_t>(cues.size());

    std::array<std::uint16_t, limits::maximumCueCount> sortedIndices{};
    std::iota(sortedIndices.begin(), sortedIndices.begin() + cueCount_, std::uint16_t{0});
    std::sort(sortedIndices.begin(), sortedIndices.begin() + cueCount_,
              [&cues](std::uint16_t left, std::uint16_t right) {
                  const auto &leftCue = cues[left];
                  const auto &rightCue = cues[right];
                  if (leftCue.getStartQuarterNotes() != rightCue.getStartQuarterNotes())
                      return leftCue.getStartQuarterNotes() < rightCue.getStartQuarterNotes();
                  if (leftCue.getEndQuarterNotes() != rightCue.getEndQuarterNotes())
                      return leftCue.getEndQuarterNotes() < rightCue.getEndQuarterNotes();
                  return left < right;
              });

    for (std::size_t index = 0; index < cueCount_; ++index)
    {
        const auto cueIndex = sortedIndices[index];
        const auto &cue = cues[cueIndex];
        startPositions_[index] = cue.getStartQuarterNotes();
        endPositions_[index] = cue.getEndQuarterNotes();
        cueIndices_[index] = cueIndex;
    }
}

std::optional<std::uint16_t> CuePlaybackIndex::findCueIndex(double positionQuarterNotes) const noexcept
{
    if (!std::isfinite(positionQuarterNotes) || positionQuarterNotes < 0.0 || cueCount_ == 0)
        return std::nullopt;

    std::size_t lower = 0;
    std::size_t upper = cueCount_;
    while (lower < upper)
    {
        const auto middle = lower + (upper - lower) / 2;
        if (startPositions_[middle] <= positionQuarterNotes)
            lower = middle + 1;
        else
            upper = middle;
    }

    if (lower == 0)
        return std::nullopt;

    const auto candidate = lower - 1;
    if (positionQuarterNotes >= endPositions_[candidate])
        return std::nullopt;

    return cueIndices_[candidate];
}

std::size_t CuePlaybackIndex::getCueCount() const noexcept
{
    return cueCount_;
}
} // namespace showlyrics
