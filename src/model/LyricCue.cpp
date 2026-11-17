// SPDX-License-Identifier: AGPL-3.0-only

#include "LyricCue.h"

#include "CueStateLimits.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace
{
bool isLowercaseHex(char character) noexcept
{
    return (character >= '0' && character <= '9') || (character >= 'a' && character <= 'f');
}

bool isVersionFourUuid(const std::string &identifier) noexcept
{
    if (identifier.size() != 36)
        return false;

    for (std::size_t index = 0; index < identifier.size(); ++index)
    {
        const auto isSeparator = index == 8 || index == 13 || index == 18 || index == 23;
        if (isSeparator ? identifier[index] != '-' : !isLowercaseHex(identifier[index]))
            return false;
    }

    const auto variant = identifier[19];
    return identifier[14] == '4' && (variant == '8' || variant == '9' || variant == 'a' || variant == 'b');
}

bool readUtf8CodePoint(const std::string &text, std::size_t &offset, std::uint32_t &codePoint) noexcept
{
    const auto first = static_cast<std::uint8_t>(text[offset]);
    std::size_t length = 0;

    if (first <= 0x7fU)
    {
        codePoint = first;
        length = 1;
    }
    else if (first >= 0xc2U && first <= 0xdfU)
    {
        codePoint = first & 0x1fU;
        length = 2;
    }
    else if (first >= 0xe0U && first <= 0xefU)
    {
        codePoint = first & 0x0fU;
        length = 3;
    }
    else if (first >= 0xf0U && first <= 0xf4U)
    {
        codePoint = first & 0x07U;
        length = 4;
    }
    else
    {
        return false;
    }

    if (offset + length > text.size())
        return false;

    for (std::size_t index = 1; index < length; ++index)
    {
        const auto next = static_cast<std::uint8_t>(text[offset + index]);
        if ((next & 0xc0U) != 0x80U)
            return false;
        codePoint = (codePoint << 6U) | (next & 0x3fU);
    }

    const auto overlong = (length == 2 && codePoint < 0x80U) || (length == 3 && codePoint < 0x800U) ||
                          (length == 4 && codePoint < 0x10000U);
    const auto surrogate = codePoint >= 0xd800U && codePoint <= 0xdfffU;
    if (overlong || surrogate || codePoint > 0x10ffffU)
        return false;

    offset += length;
    return true;
}

bool isValidPlainText(const std::string &text) noexcept
{
    std::size_t offset = 0;
    while (offset < text.size())
    {
        std::uint32_t codePoint = 0;
        if (!readUtf8CodePoint(text, offset, codePoint))
            return false;

        const auto isAllowedLineFeed = codePoint == 0x0aU;
        const auto isControl = codePoint <= 0x1fU || (codePoint >= 0x7fU && codePoint <= 0x9fU);
        if (isControl && !isAllowedLineFeed)
            return false;
    }

    return true;
}
} // namespace

namespace showlyrics
{
LyricCue::LyricCue(std::string identifier, double startQuarterNotes, double endQuarterNotes, std::string title,
                   std::string lyrics, std::uint32_t displayOrder)
    : identifier_(std::move(identifier)), startQuarterNotes_(startQuarterNotes), endQuarterNotes_(endQuarterNotes),
      title_(std::move(title)), lyrics_(std::move(lyrics)), displayOrder_(displayOrder)
{
}

LyricCueError LyricCue::validate() const noexcept
{
    if (!isVersionFourUuid(identifier_))
        return LyricCueError::invalidIdentifier;
    if (!std::isfinite(startQuarterNotes_) || !std::isfinite(endQuarterNotes_) || startQuarterNotes_ < 0.0 ||
        endQuarterNotes_ <= startQuarterNotes_)
        return LyricCueError::invalidRange;
    if (!isValidPlainText(title_))
        return LyricCueError::invalidTitleText;
    if (title_.size() > limits::maximumTitleBytes)
        return LyricCueError::titleTooLarge;
    if (lyrics_.empty())
        return LyricCueError::emptyLyrics;
    if (!isValidPlainText(lyrics_))
        return LyricCueError::invalidLyricsText;
    if (lyrics_.size() > limits::maximumLyricsBytes)
        return LyricCueError::lyricsTooLarge;

    return LyricCueError::none;
}

bool LyricCue::isValid() const noexcept
{
    return validate() == LyricCueError::none;
}

bool LyricCue::contains(double positionQuarterNotes) const noexcept
{
    return isValid() && std::isfinite(positionQuarterNotes) && positionQuarterNotes >= startQuarterNotes_ &&
           positionQuarterNotes < endQuarterNotes_;
}

const std::string &LyricCue::getIdentifier() const noexcept
{
    return identifier_;
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

std::uint32_t LyricCue::getDisplayOrder() const noexcept
{
    return displayOrder_;
}
} // namespace showlyrics
