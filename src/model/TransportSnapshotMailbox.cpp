// SPDX-License-Identifier: AGPL-3.0-only

#include "TransportSnapshotMailbox.h"

namespace showlyrics
{
TransportSnapshotPublishResult TransportSnapshotMailbox::tryPublish(const TransportSnapshot &snapshot) noexcept
{
    auto &slot = slots_[writePosition_];
    if (slot.state.load(std::memory_order_acquire) != stateValue(SlotState::empty))
    {
        overflowed_.store(true, std::memory_order_release);
        return TransportSnapshotPublishResult::full;
    }

    slot.snapshot = snapshot;
    slot.state.store(stateValue(SlotState::full), std::memory_order_release);
    writePosition_ = (writePosition_ + 1U) % slots_.size();
    return TransportSnapshotPublishResult::published;
}

bool TransportSnapshotMailbox::tryConsume(TransportSnapshot &snapshot) noexcept
{
    auto &slot = slots_[readPosition_];
    if (slot.state.load(std::memory_order_acquire) != stateValue(SlotState::full))
        return false;

    snapshot = slot.snapshot;
    slot.state.store(stateValue(SlotState::empty), std::memory_order_release);
    readPosition_ = (readPosition_ + 1U) % slots_.size();
    return true;
}

bool TransportSnapshotMailbox::consumeOverflowFlag() noexcept
{
    return overflowed_.exchange(false, std::memory_order_acq_rel);
}

std::uint8_t TransportSnapshotMailbox::stateValue(SlotState state) noexcept
{
    return static_cast<std::uint8_t>(state);
}
} // namespace showlyrics
