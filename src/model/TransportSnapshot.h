// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <cstdint>
#include <type_traits>

namespace showlyrics
{
enum class TransportSnapshotField : std::uint8_t
{
    ppqPosition = 1U << 0U,
    barStartPosition = 1U << 1U,
    tempo = 1U << 2U,
    timeSignature = 1U << 3U,
    cycleRange = 1U << 4U,
    transportState = 1U << 5U,
    activeCue = 1U << 6U
};

enum class TransportSnapshotState : std::uint8_t
{
    playing = 1U << 0U,
    recording = 1U << 1U,
    cycling = 1U << 2U
};

struct TransportSnapshot final
{
    double ppqPosition{0.0};
    double barStartQuarterNotes{0.0};
    double tempoBpm{0.0};
    double cycleStartQuarterNotes{0.0};
    double cycleEndQuarterNotes{0.0};
    std::uint64_t cueRevision{0};
    std::uint16_t timeSignatureNumerator{0};
    std::uint16_t timeSignatureDenominator{0};
    std::uint16_t activeCueIndex{0};
    std::uint8_t availableFields{0};
    std::uint8_t transportState{0};

    [[nodiscard]] bool has(TransportSnapshotField field) const noexcept
    {
        return (availableFields & static_cast<std::uint8_t>(field)) != 0;
    }

    [[nodiscard]] bool is(TransportSnapshotState state) const noexcept
    {
        return has(TransportSnapshotField::transportState) && (transportState & static_cast<std::uint8_t>(state)) != 0;
    }
};

static_assert(std::is_trivially_copyable_v<TransportSnapshot>);
static_assert(sizeof(TransportSnapshot) <= 64U);
} // namespace showlyrics
