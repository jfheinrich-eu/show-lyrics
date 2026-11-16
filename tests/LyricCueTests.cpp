// SPDX-License-Identifier: AGPL-3.0-only

#include "model/LyricCue.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

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
} // namespace

int main()
{
    const showlyrics::LyricCue cue(56.0, 88.0, "Verse 1", "First line");

    require(cue.isValid(), "a finite non-empty cue must be valid");
    require(cue.contains(56.0), "the start position must be included");
    require(cue.contains(87.999), "positions before the end must be included");
    require(!cue.contains(88.0), "the end position must be excluded");
    require(!cue.contains(std::numeric_limits<double>::quiet_NaN()), "NaN must not match a cue");

    const showlyrics::LyricCue emptyLyrics(0.0, 4.0, "Intro", "");
    require(!emptyLyrics.isValid(), "lyrics must not be empty");

    const showlyrics::LyricCue reversed(8.0, 4.0, "Invalid", "Text");
    require(!reversed.isValid(), "the end must be after the start");

    std::cout << "ShowLyricsDomainTests: PASS\n";
    return EXIT_SUCCESS;
}
