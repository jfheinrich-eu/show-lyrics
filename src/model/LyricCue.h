// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <cstdint>
#include <string>

namespace showlyrics
{
enum class LyricCueError
{
    none,
    invalidIdentifier,
    invalidRange,
    invalidTitleText,
    titleTooLarge,
    emptyLyrics,
    invalidLyricsText,
    lyricsTooLarge
};

class LyricCue final
{
  public:
    LyricCue(std::string identifier, double startQuarterNotes, double endQuarterNotes, std::string title,
             std::string lyrics, std::uint32_t displayOrder);

    [[nodiscard]] LyricCueError validate() const noexcept;
    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] bool contains(double positionQuarterNotes) const noexcept;

    [[nodiscard]] const std::string &getIdentifier() const noexcept;
    [[nodiscard]] double getStartQuarterNotes() const noexcept;
    [[nodiscard]] double getEndQuarterNotes() const noexcept;
    [[nodiscard]] const std::string &getTitle() const noexcept;
    [[nodiscard]] const std::string &getLyrics() const noexcept;
    [[nodiscard]] std::uint32_t getDisplayOrder() const noexcept;

  private:
    std::string identifier_;
    double startQuarterNotes_;
    double endQuarterNotes_;
    std::string title_;
    std::string lyrics_;
    std::uint32_t displayOrder_;
};
} // namespace showlyrics
