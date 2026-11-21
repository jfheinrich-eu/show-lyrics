// SPDX-License-Identifier: AGPL-3.0-only

#include "model/CuePlaybackIndex.h"
#include "model/CueState.h"
#include "model/CueStateLimits.h"
#include "model/LyricCue.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
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

std::string makeIdentifier(std::size_t value)
{
    std::ostringstream identifier;
    identifier << "123e4567-e89b-42d3-a456-" << std::hex << std::nouppercase << std::setw(12) << std::setfill('0')
               << value;
    return identifier.str();
}

showlyrics::LyricCue makeCue(std::size_t identifier, double start, double end, std::uint32_t displayOrder)
{
    return {makeIdentifier(identifier), start, end, "", "Original test text", displayOrder};
}

showlyrics::CueState makeState(std::vector<showlyrics::LyricCue> cues)
{
    showlyrics::CueStateValidationResult validation;
    auto state = showlyrics::CueState::create(std::move(cues), validation);
    require(state.has_value(), "test fixture state must be valid");
    return std::move(*state);
}

void requireCue(const showlyrics::CuePlaybackIndex &index, double position, std::uint16_t expectedCueIndex,
                const char *message)
{
    const auto cueIndex = index.findCueIndex(position);
    require(cueIndex.has_value() && *cueIndex == expectedCueIndex, message);
}

void testEmptyAndInvalidPositions()
{
    const showlyrics::CuePlaybackIndex emptyIndex(makeState({}));
    require(emptyIndex.getCueCount() == 0, "an empty state must create an empty index");
    require(!emptyIndex.findCueIndex(0.0), "an empty index must not match a position");

    const showlyrics::CuePlaybackIndex index(makeState({makeCue(0, 0.0, 4.0, 0)}));
    require(!index.findCueIndex(-1.0), "a negative PPQ position must not match a cue");
    require(!index.findCueIndex(std::numeric_limits<double>::quiet_NaN()), "NaN must not match a cue");
    require(!index.findCueIndex(std::numeric_limits<double>::infinity()), "positive infinity must not match a cue");
    require(!index.findCueIndex(-std::numeric_limits<double>::infinity()), "negative infinity must not match a cue");
}

void testHalfOpenBoundariesAndGaps()
{
    const showlyrics::CuePlaybackIndex index(
        makeState({makeCue(0, 0.0, 4.0, 0), makeCue(1, 4.0, 8.0, 1), makeCue(2, 10.0, 12.0, 2)}));

    requireCue(index, 0.0, 0, "the first cue start must be included");
    requireCue(index, 3.999, 0, "a position before the first cue end must match");
    requireCue(index, 4.0, 1, "an adjacent cue start must replace the previous end");
    requireCue(index, 7.999, 1, "a position before the second cue end must match");
    require(!index.findCueIndex(8.0), "a cue end must be excluded");
    require(!index.findCueIndex(9.0), "a gap must not match a cue");
    requireCue(index, 10.0, 2, "a cue start after a gap must be included");
    require(!index.findCueIndex(12.0), "the final cue end must be excluded");
}

void testDiscontinuousMovement()
{
    const showlyrics::CuePlaybackIndex index(
        makeState({makeCue(0, 0.0, 4.0, 0), makeCue(1, 4.0, 8.0, 1), makeCue(2, 8.0, 12.0, 2)}));

    requireCue(index, 1.0, 0, "initial playback position must match the first cue");
    requireCue(index, 9.0, 2, "a forward seek must resolve directly to the destination cue");
    requireCue(index, 5.0, 1, "a backward jump must resolve directly to the destination cue");
    requireCue(index, 11.9, 2, "a position before cycle wrap must match the final cue");
    requireCue(index, 4.0, 1, "a cycle wrap must resolve from the wrapped absolute position");
}

void testTimelineOrderIsIndependentOfDisplayOrder()
{
    const auto state = makeState({makeCue(0, 8.0, 12.0, 0), makeCue(1, 0.0, 4.0, 2), makeCue(2, 4.0, 8.0, 1)});
    const showlyrics::CuePlaybackIndex index(state);

    requireCue(index, 1.0, 1, "timeline lookup must retain the original cue index for the first range");
    requireCue(index, 5.0, 2, "timeline lookup must ignore display ordering for the middle range");
    requireCue(index, 9.0, 0, "timeline lookup must ignore vector and display order for the final range");
}

void testMaximumCueCount()
{
    std::vector<showlyrics::LyricCue> cues;
    cues.reserve(showlyrics::limits::maximumCueCount);
    for (std::size_t index = 0; index < showlyrics::limits::maximumCueCount; ++index)
    {
        cues.push_back(makeCue(index, static_cast<double>(index), static_cast<double>(index + 1),
                               static_cast<std::uint32_t>(index)));
    }

    const showlyrics::CuePlaybackIndex index(makeState(std::move(cues)));
    require(index.getCueCount() == showlyrics::limits::maximumCueCount,
            "the index must hold the approved maximum cue count");
    requireCue(index, 0.0, 0, "the first cue must remain searchable at maximum capacity");
    requireCue(index, 1023.5, 1023, "the final cue must remain searchable at maximum capacity");
    require(!index.findCueIndex(1024.0), "the final cue end must remain excluded at maximum capacity");
}
} // namespace

int main()
{
    testEmptyAndInvalidPositions();
    testHalfOpenBoundariesAndGaps();
    testDiscontinuousMovement();
    testTimelineOrderIsIndependentOfDisplayOrder();
    testMaximumCueCount();

    std::cout << "ShowLyricsCuePlaybackIndexTests: PASS\n";
    return EXIT_SUCCESS;
}
