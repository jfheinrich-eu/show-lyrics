// SPDX-License-Identifier: AGPL-3.0-only

#include "CueState.h"

#include "CueStateLimits.h"

#include <algorithm>
#include <numeric>
#include <unordered_set>
#include <utility>

namespace showlyrics
{
std::optional<CueState> CueState::create(std::vector<LyricCue> cues, CueStateValidationResult &validationResult)
{
    validationResult = validate(cues);
    if (!validationResult)
        return std::nullopt;

    return CueState(std::move(cues));
}

CueStateValidationResult CueState::validate(const std::vector<LyricCue> &cues)
{
    if (cues.size() > limits::maximumCueCount)
        return {CueStateError::tooManyCues};

    std::unordered_set<std::string> identifiers;
    std::vector<bool> displayOrders(cues.size(), false);
    std::size_t combinedTextBytes = 0;

    for (std::size_t index = 0; index < cues.size(); ++index)
    {
        const auto &cue = cues[index];
        const auto cueError = cue.validate();
        if (cueError != LyricCueError::none)
            return {CueStateError::invalidCue, cueError, index};

        if (!identifiers.insert(cue.getIdentifier()).second)
            return {CueStateError::duplicateIdentifier, LyricCueError::none, index};

        const auto displayOrder = static_cast<std::size_t>(cue.getDisplayOrder());
        if (displayOrder >= cues.size() || displayOrders[displayOrder])
            return {CueStateError::invalidDisplayOrder, LyricCueError::none, index};
        displayOrders[displayOrder] = true;

        const auto textBytes = cue.getTitle().size() + cue.getLyrics().size();
        if (textBytes > limits::maximumCombinedTextBytes - combinedTextBytes)
            return {CueStateError::combinedTextTooLarge, LyricCueError::none, index};
        combinedTextBytes += textBytes;
    }

    std::vector<std::size_t> rangeOrder(cues.size());
    std::iota(rangeOrder.begin(), rangeOrder.end(), 0);
    std::sort(rangeOrder.begin(), rangeOrder.end(), [&cues](std::size_t left, std::size_t right) {
        return cues[left].getStartQuarterNotes() < cues[right].getStartQuarterNotes();
    });

    for (std::size_t index = 1; index < rangeOrder.size(); ++index)
    {
        const auto previous = rangeOrder[index - 1];
        const auto current = rangeOrder[index];
        if (cues[current].getStartQuarterNotes() < cues[previous].getEndQuarterNotes())
            return {CueStateError::overlappingRanges, LyricCueError::none, current, previous};
    }

    return {};
}

const std::vector<LyricCue> &CueState::getCues() const noexcept
{
    return cues_;
}

CueState::CueState(std::vector<LyricCue> cues) : cues_(std::move(cues))
{
}
} // namespace showlyrics
