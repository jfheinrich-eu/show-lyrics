// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <cstddef>

namespace showlyrics::limits
{
inline constexpr std::size_t maximumCueCount = 1024;
inline constexpr std::size_t maximumTitleBytes = 256;
inline constexpr std::size_t maximumLyricsBytes = 16 * 1024;
inline constexpr std::size_t maximumCombinedTextBytes = 512 * 1024;
inline constexpr std::size_t maximumSerializedStateBytes = 1024 * 1024;
} // namespace showlyrics::limits
