// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "CueState.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace showlyrics
{
struct CueStateSerializationResult final
{
    CueStateError error{CueStateError::none};
    std::vector<std::uint8_t> data;

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return error == CueStateError::none;
    }
};

struct CueStateDeserializationResult final
{
    CueStateError error{CueStateError::none};
    CueStateValidationResult validation;
    std::optional<CueState> state;

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return error == CueStateError::none && state.has_value();
    }
};

class CueStateSerializer final
{
  public:
    static constexpr int schemaVersion = 1;

    [[nodiscard]] static CueStateSerializationResult serialize(const CueState &state);
    [[nodiscard]] static CueStateDeserializationResult deserialize(const void *data, std::size_t sizeInBytes);
};
} // namespace showlyrics
