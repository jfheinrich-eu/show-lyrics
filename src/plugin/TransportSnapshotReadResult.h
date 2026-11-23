// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "model/TransportSnapshot.h"

namespace showlyrics
{
struct TransportSnapshotReadResult final
{
    TransportSnapshot snapshot{};
    bool hasSnapshot{false};
    bool overflowed{false};
    bool audioProcessingAvailable{false};

    [[nodiscard]] bool canDisplayActiveCue() const noexcept
    {
        return hasSnapshot && !overflowed && audioProcessingAvailable &&
               snapshot.has(TransportSnapshotField::activeCue);
    }
};
} // namespace showlyrics
