// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "TransportSnapshot.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace showlyrics
{
enum class TransportSnapshotPublishResult
{
    published,
    full
};

// Bounded single-producer/single-consumer mailbox. The audio thread is the only
// producer and the editor/message thread is the only consumer. A full mailbox
// rejects the new snapshot, returns full, and latches an overflow flag. The
// consumer must treat that flag as a request to resynchronise instead of assuming
// that every safety-relevant transport transition was observed. A release store
// publishes a completed snapshot; the consumer's acquire load precedes its copy.
// The consumer releases an emptied slot before the producer may write it again.
class TransportSnapshotMailbox final
{
  public:
    static constexpr std::size_t capacity = 8;

    TransportSnapshotMailbox() = default;
    ~TransportSnapshotMailbox() = default;

    TransportSnapshotMailbox(const TransportSnapshotMailbox &) = delete;
    TransportSnapshotMailbox &operator=(const TransportSnapshotMailbox &) = delete;
    TransportSnapshotMailbox(TransportSnapshotMailbox &&) = delete;
    TransportSnapshotMailbox &operator=(TransportSnapshotMailbox &&) = delete;

    [[nodiscard]] TransportSnapshotPublishResult tryPublish(const TransportSnapshot &snapshot) noexcept;
    [[nodiscard]] bool tryConsume(TransportSnapshot &snapshot) noexcept;

    // Returns and clears the sticky overflow indication. A concurrent failure
    // after the exchange remains latched for the next call.
    [[nodiscard]] bool consumeOverflowFlag() noexcept;

  private:
    enum class SlotState : std::uint8_t
    {
        empty,
        full
    };

    struct Slot final
    {
        std::atomic<std::uint8_t> state{static_cast<std::uint8_t>(SlotState::empty)};
        TransportSnapshot snapshot{};
    };

    [[nodiscard]] static std::uint8_t stateValue(SlotState state) noexcept;

    static_assert(std::atomic<std::uint8_t>::is_always_lock_free);
    static_assert(std::atomic<bool>::is_always_lock_free);

    std::array<Slot, capacity> slots_{};
    std::atomic<bool> overflowed_{false};
    std::size_t writePosition_{0}; // Audio-thread only.
    std::size_t readPosition_{0};  // Editor/message-thread only.
};
} // namespace showlyrics
