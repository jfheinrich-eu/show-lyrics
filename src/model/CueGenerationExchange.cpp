// SPDX-License-Identifier: AGPL-3.0-only

#include "CueGenerationExchange.h"

#include <utility>

namespace showlyrics
{
CueGenerationPublishResult CueGenerationExchange::tryPublish(std::unique_ptr<const CueState> &cueState,
                                                             std::uint64_t revision)
{
    if (!cueState)
        return CueGenerationPublishResult::invalidState;
    if (revision == 0)
        return CueGenerationPublishResult::invalidRevision;
    if (revision <= lastPublishedRevision_)
        return CueGenerationPublishResult::staleRevision;

    Slot *availableSlot = nullptr;
    for (auto &slot : slots_)
    {
        auto expected = stateValue(SlotState::free);
        if (slot.state.compare_exchange_strong(expected, stateValue(SlotState::writing), std::memory_order_acquire,
                                               std::memory_order_relaxed))
        {
            availableSlot = &slot;
            break;
        }
    }

    if (availableSlot == nullptr)
        return CueGenerationPublishResult::slotsFull;

    try
    {
        availableSlot->playbackIndex.emplace(*cueState);
        availableSlot->cueState = std::move(cueState);
        availableSlot->revision = revision;
    }
    catch (...)
    {
        availableSlot->playbackIndex.reset();
        availableSlot->cueState.reset();
        availableSlot->revision = 0;
        availableSlot->state.store(stateValue(SlotState::free), std::memory_order_release);
        throw;
    }

    lastPublishedRevision_ = revision;
    availableSlot->state.store(stateValue(SlotState::ready), std::memory_order_release);
    return CueGenerationPublishResult::published;
}

const CueState *CueGenerationExchange::findCueState(std::uint64_t revision) const noexcept
{
    if (revision == 0)
        return nullptr;

    for (const auto &slot : slots_)
    {
        const auto state = slot.state.load(std::memory_order_acquire);
        if (state != stateValue(SlotState::free) && state != stateValue(SlotState::writing) &&
            state != stateValue(SlotState::reclaiming) && slot.revision == revision)
        {
            return slot.cueState.get();
        }
    }

    return nullptr;
}

std::size_t CueGenerationExchange::reclaimRetired() noexcept
{
    std::size_t reclaimed = 0;
    for (auto &slot : slots_)
    {
        auto expected = stateValue(SlotState::retired);
        if (!slot.state.compare_exchange_strong(expected, stateValue(SlotState::reclaiming), std::memory_order_acquire,
                                                std::memory_order_relaxed))
        {
            continue;
        }

        slot.playbackIndex.reset();
        slot.cueState.reset();
        slot.revision = 0;
        slot.state.store(stateValue(SlotState::free), std::memory_order_release);
        ++reclaimed;
    }

    return reclaimed;
}

CueGenerationAudioView CueGenerationExchange::activateLatestForAudio() noexcept
{
    std::size_t selectedSlot = slotCount;
    auto selectedRevision = activeRevision_;

    for (std::size_t index = 0; index < slots_.size(); ++index)
    {
        const auto &slot = slots_[index];
        if (slot.state.load(std::memory_order_acquire) == stateValue(SlotState::ready) &&
            slot.revision > selectedRevision)
        {
            selectedSlot = index;
            selectedRevision = slot.revision;
        }
    }

    if (selectedSlot != slotCount)
    {
        auto expected = stateValue(SlotState::ready);
        if (slots_[selectedSlot].state.compare_exchange_strong(expected, stateValue(SlotState::active),
                                                               std::memory_order_acquire, std::memory_order_relaxed))
        {
            const auto previousSlot = activeSlot_;
            activeSlot_ = selectedSlot;
            activeRevision_ = selectedRevision;

            if (previousSlot != slotCount)
                slots_[previousSlot].state.store(stateValue(SlotState::retired), std::memory_order_release);
        }
    }

    retireReadyAtOrBelow(activeRevision_);
    return currentAudioView();
}

CueGenerationAudioView CueGenerationExchange::currentAudioView() const noexcept
{
    if (activeSlot_ == slotCount)
        return {};

    return {&*slots_[activeSlot_].playbackIndex, activeRevision_};
}

std::uint8_t CueGenerationExchange::stateValue(SlotState state) noexcept
{
    return static_cast<std::uint8_t>(state);
}

void CueGenerationExchange::retireReadyAtOrBelow(std::uint64_t revision) noexcept
{
    if (revision == 0)
        return;

    for (auto &slot : slots_)
    {
        if (slot.state.load(std::memory_order_acquire) != stateValue(SlotState::ready) || slot.revision > revision)
        {
            continue;
        }

        auto expected = stateValue(SlotState::ready);
        static_cast<void>(slot.state.compare_exchange_strong(expected, stateValue(SlotState::retired),
                                                             std::memory_order_release, std::memory_order_relaxed));
    }
}
} // namespace showlyrics
