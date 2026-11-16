// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <string>

namespace showlyrics
{
class LyricCue final
{
  public:
    LyricCue(double startQuarterNotes, double endQuarterNotes, std::string title, std::string lyrics);

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] bool contains(double positionQuarterNotes) const noexcept;

    [[nodiscard]] double getStartQuarterNotes() const noexcept;
    [[nodiscard]] double getEndQuarterNotes() const noexcept;
    [[nodiscard]] const std::string &getTitle() const noexcept;
    [[nodiscard]] const std::string &getLyrics() const noexcept;

  private:
    double startQuarterNotes_;
    double endQuarterNotes_;
    std::string title_;
    std::string lyrics_;
};
} // namespace showlyrics
