// SPDX-License-Identifier: AGPL-3.0-only

#include "LyricCue.h"

#include <cmath>
#include <utility>

namespace showlyrics
{
LyricCue::LyricCue(double startQuarterNotes, double endQuarterNotes, std::string title, std::string lyrics)
    : startQuarterNotes_(startQuarterNotes), endQuarterNotes_(endQuarterNotes), title_(std::move(title)),
      lyrics_(std::move(lyrics))
{
}

bool LyricCue::isValid() const noexcept
{
    return std::isfinite(startQuarterNotes_) && std::isfinite(endQuarterNotes_) && startQuarterNotes_ >= 0.0 &&
           endQuarterNotes_ > startQuarterNotes_ && !lyrics_.empty();
}

bool LyricCue::contains(double positionQuarterNotes) const noexcept
{
    return isValid() && std::isfinite(positionQuarterNotes) && positionQuarterNotes >= startQuarterNotes_ &&
           positionQuarterNotes < endQuarterNotes_;
}

double LyricCue::getStartQuarterNotes() const noexcept
{
    return startQuarterNotes_;
}

double LyricCue::getEndQuarterNotes() const noexcept
{
    return endQuarterNotes_;
}

const std::string &LyricCue::getTitle() const noexcept
{
    return title_;
}

const std::string &LyricCue::getLyrics() const noexcept
{
    return lyrics_;
}
} // namespace showlyrics
