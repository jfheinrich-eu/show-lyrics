// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "LyricCue.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace showlyrics
{
enum class CueStateError
{
    none,
    tooManyCues,
    invalidCue,
    duplicateIdentifier,
    invalidDisplayOrder,
    combinedTextTooLarge,
    overlappingRanges,
    serializedStateTooLarge,
    malformedState,
    unsupportedSchemaVersion
};

struct CueStateValidationResult final
{
    CueStateError error{CueStateError::none};
    LyricCueError cueError{LyricCueError::none};
    std::size_t cueIndex{0};
    std::size_t conflictingCueIndex{0};

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return error == CueStateError::none;
    }
};

class CueState final
{
  public:
    [[nodiscard]] static std::optional<CueState> create(std::vector<LyricCue> cues,
                                                        CueStateValidationResult &validationResult);
    [[nodiscard]] static CueStateValidationResult validate(const std::vector<LyricCue> &cues);

    [[nodiscard]] const std::vector<LyricCue> &getCues() const noexcept;

  private:
    explicit CueState(std::vector<LyricCue> cues);

    std::vector<LyricCue> cues_;
};
} // namespace showlyrics
