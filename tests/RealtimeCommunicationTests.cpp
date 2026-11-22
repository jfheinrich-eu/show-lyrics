// SPDX-License-Identifier: AGPL-3.0-only

#include "model/CueGenerationExchange.h"
#include "model/CueState.h"
#include "model/LyricCue.h"
#include "model/TransportSnapshot.h"
#include "model/TransportSnapshotMailbox.h"

#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{
void require(bool condition, const char *message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::string makeIdentifier(std::uint64_t revision)
{
    std::ostringstream identifier;
    identifier << "123e4567-e89b-42d3-a456-" << std::hex << std::nouppercase << std::setw(12) << std::setfill('0')
               << revision;
    return identifier.str();
}

std::unique_ptr<const showlyrics::CueState> makeState(std::uint64_t revision)
{
    std::vector<showlyrics::LyricCue> cues;
    const auto start = static_cast<double>(revision) * 2.0;
    cues.emplace_back(makeIdentifier(revision), start, start + 1.0, "", "Original test text", 0);

    showlyrics::CueStateValidationResult validation;
    auto state = showlyrics::CueState::create(std::move(cues), validation);
    require(state.has_value(), "test fixture state must be valid");
    return std::make_unique<const showlyrics::CueState>(std::move(*state));
}

void requireGenerationMatches(const showlyrics::CueGenerationAudioView &view, std::uint64_t revision,
                              const char *message)
{
    if (!view || view.revision != revision)
    {
        require(false, message);
        return;
    }

    const auto position = static_cast<double>(revision) * 2.0 + 0.5;
    const auto cue = view.playbackIndex->findCueIndex(position);
    require(cue.has_value() && *cue == 0, message);
}

void testGenerationOwnershipAndRevisionTransitions()
{
    showlyrics::CueGenerationExchange exchange;
    std::unique_ptr<const showlyrics::CueState> missingState;
    require(exchange.tryPublish(missingState, 1) == showlyrics::CueGenerationPublishResult::invalidState,
            "a missing state must be rejected");

    auto firstState = makeState(1);
    require(exchange.tryPublish(firstState, 0) == showlyrics::CueGenerationPublishResult::invalidRevision,
            "revision zero must be rejected");
    require(firstState != nullptr, "a rejected publication must retain caller ownership");
    require(exchange.tryPublish(firstState, 1) == showlyrics::CueGenerationPublishResult::published,
            "the first generation must be published");
    require(firstState == nullptr, "a successful publication must transfer ownership");
    require(exchange.findCueState(1) != nullptr, "the control thread must retain the published CueState");

    const auto firstView = exchange.activateLatestForAudio();
    requireGenerationMatches(firstView, 1, "the first generation must activate atomically");

    auto staleState = makeState(2);
    require(exchange.tryPublish(staleState, 1) == showlyrics::CueGenerationPublishResult::staleRevision,
            "a reused revision must be rejected");
    require(staleState != nullptr, "a stale revision must not consume the caller's state");

    auto secondState = makeState(2);
    require(exchange.tryPublish(secondState, 2) == showlyrics::CueGenerationPublishResult::published,
            "a newer generation must be published");
    const auto secondView = exchange.activateLatestForAudio();
    requireGenerationMatches(secondView, 2, "revision and playback index must switch together");
    require(exchange.findCueState(1) != nullptr, "a retired state must live until control-thread reclamation");
    require(exchange.reclaimRetired() == 1, "the control thread must reclaim the retired generation");
    require(exchange.findCueState(1) == nullptr, "a reclaimed state must no longer be accessible");
    require(exchange.findCueState(2) != nullptr, "the active state must not be reclaimed");
}

void testGenerationCapacityAndSlotReuse()
{
    showlyrics::CueGenerationExchange exchange;
    for (std::uint64_t revision = 1; revision <= showlyrics::CueGenerationExchange::slotCount; ++revision)
    {
        auto state = makeState(revision);
        require(exchange.tryPublish(state, revision) == showlyrics::CueGenerationPublishResult::published,
                "each free generation slot must accept one publication");
    }

    auto blockedState = makeState(4);
    require(exchange.tryPublish(blockedState, 4) == showlyrics::CueGenerationPublishResult::slotsFull,
            "publication must fail explicitly when every generation slot is occupied");
    require(blockedState != nullptr, "a full exchange must retain caller ownership");

    const auto newestView = exchange.activateLatestForAudio();
    requireGenerationMatches(newestView, 3, "the audio thread must choose the newest ready generation");
    require(exchange.reclaimRetired() == 2, "older ready generations must become reclaimable");

    require(exchange.tryPublish(blockedState, 4) == showlyrics::CueGenerationPublishResult::published,
            "a reclaimed slot must accept the previously blocked generation");
    const auto reusedView = exchange.activateLatestForAudio();
    requireGenerationMatches(reusedView, 4, "slot reuse must not combine a new revision with an old index");
    require(!reusedView.playbackIndex->findCueIndex(6.5),
            "a reused slot must not expose the previous generation's cue range");
}

void testConcurrentGenerationTakeovers()
{
    constexpr std::uint64_t finalRevision = 500;
    showlyrics::CueGenerationExchange exchange;
    std::atomic<bool> producerFinished{false};
    std::atomic<bool> failed{false};

    std::thread producer([&] {
        for (std::uint64_t revision = 1; revision <= finalRevision && !failed.load(); ++revision)
        {
            auto state = makeState(revision);
            bool published = false;
            for (std::size_t attempt = 0; attempt < 1'000'000 && !published; ++attempt)
            {
                const auto result = exchange.tryPublish(state, revision);
                if (result == showlyrics::CueGenerationPublishResult::published)
                {
                    published = true;
                }
                else if (result == showlyrics::CueGenerationPublishResult::slotsFull)
                {
                    static_cast<void>(exchange.reclaimRetired());
                    std::this_thread::yield();
                }
                else
                {
                    failed.store(true);
                }
            }
            if (!published)
                failed.store(true);
            static_cast<void>(exchange.reclaimRetired());
        }
        producerFinished.store(true, std::memory_order_release);
    });

    std::uint64_t observedRevision = 0;
    for (std::size_t attempt = 0; attempt < 2'000'000 && !failed.load(); ++attempt)
    {
        const auto view = exchange.activateLatestForAudio();
        if (view)
        {
            if (view.revision < observedRevision)
            {
                failed.store(true);
                break;
            }

            const auto expectedPosition = static_cast<double>(view.revision) * 2.0 + 0.5;
            const auto cue = view.playbackIndex->findCueIndex(expectedPosition);
            if (!cue || *cue != 0)
            {
                failed.store(true);
                break;
            }
            observedRevision = view.revision;
        }

        if (producerFinished.load(std::memory_order_acquire) && observedRevision == finalRevision)
            break;
        std::this_thread::yield();
    }

    producer.join();
    require(!failed.load(), "concurrent generation publication and takeover must remain consistent");
    require(observedRevision == finalRevision, "the audio thread must eventually activate the final revision");
}

showlyrics::TransportSnapshot makeSnapshot(std::uint64_t sequence)
{
    showlyrics::TransportSnapshot snapshot;
    snapshot.ppqPosition = static_cast<double>(sequence) * 0.5;
    snapshot.barStartQuarterNotes = static_cast<double>(sequence) * 2.0;
    snapshot.tempoBpm = 60.0 + static_cast<double>(sequence % 120);
    snapshot.cycleStartQuarterNotes = static_cast<double>(sequence);
    snapshot.cycleEndQuarterNotes = static_cast<double>(sequence) + 8.0;
    snapshot.cueRevision = sequence;
    snapshot.timeSignatureNumerator = 7;
    snapshot.timeSignatureDenominator = 8;
    snapshot.activeCueIndex = static_cast<std::uint16_t>(sequence % 1024);
    snapshot.availableFields = static_cast<std::uint8_t>(showlyrics::TransportSnapshotField::ppqPosition) |
                               static_cast<std::uint8_t>(showlyrics::TransportSnapshotField::barStartPosition) |
                               static_cast<std::uint8_t>(showlyrics::TransportSnapshotField::tempo) |
                               static_cast<std::uint8_t>(showlyrics::TransportSnapshotField::timeSignature) |
                               static_cast<std::uint8_t>(showlyrics::TransportSnapshotField::cycleRange) |
                               static_cast<std::uint8_t>(showlyrics::TransportSnapshotField::transportState) |
                               static_cast<std::uint8_t>(showlyrics::TransportSnapshotField::activeCue);
    snapshot.transportState = static_cast<std::uint8_t>(showlyrics::TransportSnapshotState::playing) |
                              static_cast<std::uint8_t>(showlyrics::TransportSnapshotState::cycling);
    return snapshot;
}

bool isConsistentSnapshot(const showlyrics::TransportSnapshot &snapshot, std::uint64_t sequence)
{
    return snapshot.cueRevision == sequence && snapshot.ppqPosition == static_cast<double>(sequence) * 0.5 &&
           snapshot.barStartQuarterNotes == static_cast<double>(sequence) * 2.0 &&
           snapshot.tempoBpm == 60.0 + static_cast<double>(sequence % 120) &&
           snapshot.cycleStartQuarterNotes == static_cast<double>(sequence) &&
           snapshot.cycleEndQuarterNotes == static_cast<double>(sequence) + 8.0 &&
           snapshot.activeCueIndex == sequence % 1024 && snapshot.timeSignatureNumerator == 7 &&
           snapshot.timeSignatureDenominator == 8;
}

void testSnapshotPresenceAndMailboxCapacity()
{
    showlyrics::TransportSnapshot missingValues;
    require(!missingValues.has(showlyrics::TransportSnapshotField::ppqPosition),
            "a default snapshot must identify a missing PPQ position");
    require(!missingValues.has(showlyrics::TransportSnapshotField::transportState),
            "a default snapshot must identify missing transport state");
    require(!missingValues.has(showlyrics::TransportSnapshotField::activeCue),
            "a default snapshot must identify a missing active cue");
    require(!missingValues.is(showlyrics::TransportSnapshotState::playing),
            "missing transport state must not be interpreted as playing");

    showlyrics::TransportSnapshotMailbox mailbox;
    for (std::uint64_t sequence = 1; sequence <= showlyrics::TransportSnapshotMailbox::capacity; ++sequence)
    {
        require(mailbox.tryPublish(makeSnapshot(sequence)) == showlyrics::TransportSnapshotPublishResult::published,
                "each empty mailbox slot must accept a snapshot");
    }

    showlyrics::TransportSnapshot stoppedSnapshot;
    stoppedSnapshot.availableFields = static_cast<std::uint8_t>(showlyrics::TransportSnapshotField::transportState);
    require(stoppedSnapshot.has(showlyrics::TransportSnapshotField::transportState) &&
                !stoppedSnapshot.is(showlyrics::TransportSnapshotState::playing) &&
                !stoppedSnapshot.has(showlyrics::TransportSnapshotField::activeCue),
            "a known stopped state must be representable without an active cue");
    require(mailbox.tryPublish(stoppedSnapshot) == showlyrics::TransportSnapshotPublishResult::full,
            "a full mailbox must explicitly reject a safety-relevant transport update");
    require(mailbox.consumeOverflowFlag(), "a rejected publication must latch an overflow indication");
    require(!mailbox.consumeOverflowFlag(), "reading the overflow indication must clear it");

    for (std::uint64_t sequence = 1; sequence <= showlyrics::TransportSnapshotMailbox::capacity; ++sequence)
    {
        showlyrics::TransportSnapshot snapshot;
        require(mailbox.tryConsume(snapshot), "a published snapshot must be consumable");
        require(isConsistentSnapshot(snapshot, sequence), "mailbox consumption must preserve FIFO snapshots");
    }

    showlyrics::TransportSnapshot snapshot;
    require(!mailbox.tryConsume(snapshot), "an empty mailbox must not produce a snapshot");
}

void testConcurrentMailboxWithSlowReader()
{
    constexpr std::uint64_t finalSequence = 10'000;
    showlyrics::TransportSnapshotMailbox mailbox;
    std::atomic<bool> producerFinished{false};
    std::atomic<bool> producerObservedFull{false};
    std::atomic<bool> failed{false};

    std::thread producer([&] {
        for (std::uint64_t sequence = 1; sequence <= finalSequence && !failed.load(); ++sequence)
        {
            const auto snapshot = makeSnapshot(sequence);
            bool published = false;
            for (std::size_t attempt = 0; attempt < 1'000'000 && !published && !failed.load(); ++attempt)
            {
                if (mailbox.tryPublish(snapshot) == showlyrics::TransportSnapshotPublishResult::published)
                    published = true;
                else
                {
                    producerObservedFull.store(true, std::memory_order_release);
                    std::this_thread::yield();
                }
            }
            if (!published)
                failed.store(true);
        }
        producerFinished.store(true, std::memory_order_release);
    });

    for (std::size_t attempt = 0; attempt < 1'000'000 && !producerObservedFull.load(std::memory_order_acquire);
         ++attempt)
    {
        std::this_thread::yield();
    }
    if (!producerObservedFull.load(std::memory_order_acquire))
        failed.store(true);

    std::uint64_t expectedSequence = 1;
    for (std::size_t attempt = 0; attempt < 10'000'000 && !failed.load(); ++attempt)
    {
        showlyrics::TransportSnapshot snapshot;
        if (mailbox.tryConsume(snapshot))
        {
            if (!isConsistentSnapshot(snapshot, expectedSequence))
            {
                failed.store(true);
                break;
            }
            ++expectedSequence;

            if (expectedSequence % 97 == 0)
            {
                for (int pause = 0; pause < 100; ++pause)
                    std::this_thread::yield();
            }
        }
        else
        {
            if (producerFinished.load(std::memory_order_acquire) && expectedSequence > finalSequence)
                break;
            std::this_thread::yield();
        }
    }

    producer.join();
    require(!failed.load(), "concurrent mailbox access must not tear or reorder snapshots");
    require(expectedSequence == finalSequence + 1, "the slow reader must eventually consume every retried snapshot");
    require(mailbox.consumeOverflowFlag(), "the deliberately blocked reader must observe mailbox exhaustion");
}
} // namespace

int main()
{
    testGenerationOwnershipAndRevisionTransitions();
    testGenerationCapacityAndSlotReuse();
    testConcurrentGenerationTakeovers();
    testSnapshotPresenceAndMailboxCapacity();
    testConcurrentMailboxWithSlowReader();

    std::cout << "ShowLyricsRealtimeCommunicationTests: PASS\n";
    return EXIT_SUCCESS;
}
