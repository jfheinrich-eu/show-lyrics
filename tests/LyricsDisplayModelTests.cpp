// SPDX-License-Identifier: AGPL-3.0-only

#include "plugin/LyricsDisplayModel.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

namespace
{
using showlyrics::ActiveCueReference;
using showlyrics::LyricsDisplayMode;
using showlyrics::LyricsDisplayModel;
using showlyrics::TransportSnapshotField;
using showlyrics::TransportSnapshotReadResult;
using showlyrics::TransportSnapshotState;

void require(bool condition, const char *message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

TransportSnapshotReadResult makeResult()
{
    TransportSnapshotReadResult result;
    result.audioProcessingAvailable = true;
    return result;
}

TransportSnapshotReadResult makeActiveResult(std::uint64_t revision, std::uint16_t cueIndex)
{
    auto result = makeResult();
    result.hasSnapshot = true;
    result.snapshot.cueRevision = revision;
    result.snapshot.activeCueIndex = cueIndex;
    result.snapshot.availableFields = static_cast<std::uint8_t>(TransportSnapshotField::transportState) |
                                      static_cast<std::uint8_t>(TransportSnapshotField::ppqPosition) |
                                      static_cast<std::uint8_t>(TransportSnapshotField::activeCue);
    result.snapshot.transportState = static_cast<std::uint8_t>(TransportSnapshotState::playing);
    return result;
}

TransportSnapshotReadResult makeStoppedResult()
{
    auto result = makeResult();
    result.hasSnapshot = true;
    result.snapshot.availableFields = static_cast<std::uint8_t>(TransportSnapshotField::transportState) |
                                      static_cast<std::uint8_t>(TransportSnapshotField::ppqPosition);
    return result;
}

TransportSnapshotReadResult makeNoCueResult()
{
    auto result = makeStoppedResult();
    result.snapshot.transportState = static_cast<std::uint8_t>(TransportSnapshotState::playing);
    return result;
}

void testActiveCueResolutionAndUnchangedUpdates()
{
    const LyricsDisplayModel::TimePoint openedAt{};
    LyricsDisplayModel model(openedAt);
    auto resolverCalls = 0;
    const auto resolver = [&](ActiveCueReference cue) -> std::optional<std::string> {
        ++resolverCalls;
        return "Lyrics " + std::to_string(cue.revision) + ':' + std::to_string(cue.cueIndex);
    };

    require(model.update(makeActiveResult(1, 0), openedAt + std::chrono::milliseconds{10}, resolver),
            "the first active snapshot must change the initial display state");
    require(model.getMode() == LyricsDisplayMode::staleSnapshot && model.getLyrics().empty(),
            "the first active snapshot after opening must remain hidden");
    require(resolverCalls == 0, "an unconfirmed snapshot must not access cue text");

    require(model.update(makeActiveResult(1, 0), openedAt + std::chrono::milliseconds{20}, resolver),
            "a second active snapshot must confirm live audio processing");
    require(model.getMode() == LyricsDisplayMode::activeCue && model.getLyrics() == "Lyrics 1:0",
            "a confirmed active cue must expose its copied lyric text");
    require(resolverCalls == 1, "a newly active cue must be resolved exactly once");

    require(!model.update(makeActiveResult(1, 0), openedAt + std::chrono::milliseconds{30}, resolver),
            "an unchanged cue must not request a UI update");
    require(resolverCalls == 1, "an unchanged cue must not copy lyrics again");
}

void testStoppedInvalidHostAndNoCueStates()
{
    const LyricsDisplayModel::TimePoint openedAt{};
    LyricsDisplayModel model(openedAt);
    const auto resolver = [](ActiveCueReference) -> std::optional<std::string> { return "Lyrics"; };

    static_cast<void>(model.update(makeStoppedResult(), openedAt + std::chrono::milliseconds{10}, resolver));
    require(model.getMode() == LyricsDisplayMode::stopped && model.getLyrics().empty(),
            "an observed stop must hide lyric text");

    auto missingPosition = makeResult();
    missingPosition.hasSnapshot = true;
    missingPosition.snapshot.availableFields = static_cast<std::uint8_t>(TransportSnapshotField::transportState);
    missingPosition.snapshot.transportState = static_cast<std::uint8_t>(TransportSnapshotState::playing);
    static_cast<void>(model.update(missingPosition, openedAt + std::chrono::milliseconds{20}, resolver));
    require(model.getMode() == LyricsDisplayMode::invalidHostPosition,
            "playing without PPQ must be an invalid host position");

    static_cast<void>(model.update(makeNoCueResult(), openedAt + std::chrono::milliseconds{30}, resolver));
    require(model.getMode() == LyricsDisplayMode::noCue,
            "a valid playing position outside cue boundaries must be neutral");

    auto missingTransport = makeResult();
    missingTransport.hasSnapshot = true;
    static_cast<void>(model.update(missingTransport, openedAt + std::chrono::milliseconds{40}, resolver));
    require(model.getMode() == LyricsDisplayMode::invalidHostPosition,
            "missing transport status must invalidate host position");
}

void testStaleSnapshotIsDistinctFromStopped()
{
    const LyricsDisplayModel::TimePoint openedAt{};
    LyricsDisplayModel model(openedAt);
    const auto resolver = [](ActiveCueReference) -> std::optional<std::string> { return "Lyrics"; };

    static_cast<void>(model.update(makeStoppedResult(), openedAt + std::chrono::milliseconds{10}, resolver));
    auto noSnapshot = makeResult();
    require(!model.update(noSnapshot, openedAt + std::chrono::milliseconds{509}, resolver),
            "a stopped snapshot must remain known before the stale timeout");
    require(model.getMode() == LyricsDisplayMode::stopped, "stopped and stale must remain distinct states");

    require(model.update(noSnapshot, openedAt + std::chrono::milliseconds{510}, resolver),
            "the exact timeout must invalidate an old snapshot");
    require(model.getMode() == LyricsDisplayMode::staleSnapshot,
            "missing callbacks must become stale rather than proving a stop");

    LyricsDisplayModel neverUpdated(openedAt);
    static_cast<void>(neverUpdated.update(noSnapshot, openedAt + LyricsDisplayModel::staleSnapshotTimeout, resolver));
    require(neverUpdated.getMode() == LyricsDisplayMode::staleSnapshot,
            "an editor without any snapshot must become stale after the timeout");
}

void testOverflowAndAudioLifecycleInvalidation()
{
    const LyricsDisplayModel::TimePoint openedAt{};
    LyricsDisplayModel model(openedAt);
    const auto resolver = [](ActiveCueReference cue) -> std::optional<std::string> {
        return "Lyrics " + std::to_string(cue.revision);
    };

    static_cast<void>(model.update(makeActiveResult(1, 0), openedAt + std::chrono::milliseconds{10}, resolver));
    static_cast<void>(model.update(makeActiveResult(1, 0), openedAt + std::chrono::milliseconds{20}, resolver));
    require(model.getMode() == LyricsDisplayMode::activeCue, "test setup must display an active cue");

    auto overflowed = makeActiveResult(1, 0);
    overflowed.overflowed = true;
    static_cast<void>(model.update(overflowed, openedAt + std::chrono::milliseconds{30}, resolver));
    require(model.getMode() == LyricsDisplayMode::mailboxOverflow && model.getLyrics().empty(),
            "overflow must win over concurrently consumed active snapshots");

    static_cast<void>(model.update(makeActiveResult(1, 0), openedAt + std::chrono::milliseconds{40}, resolver));
    require(model.getMode() == LyricsDisplayMode::activeCue,
            "a later safe snapshot must allow the cue to be resolved again");

    auto unavailable = makeResult();
    unavailable.audioProcessingAvailable = false;
    static_cast<void>(model.update(unavailable, openedAt + std::chrono::milliseconds{50}, resolver));
    require(model.getMode() == LyricsDisplayMode::audioUnavailable && model.getLyrics().empty(),
            "unavailable audio processing must immediately clear lyrics");
}

void testRevisionChangesAndInvalidCueReferences()
{
    const LyricsDisplayModel::TimePoint openedAt{};
    LyricsDisplayModel model(openedAt);
    auto resolverCalls = 0;
    const auto resolver = [&](ActiveCueReference cue) -> std::optional<std::string> {
        ++resolverCalls;
        if (cue.revision == 3 || cue.cueIndex >= 2)
            return std::nullopt;
        return "Revision " + std::to_string(cue.revision);
    };

    static_cast<void>(model.update(makeActiveResult(1, 0), openedAt + std::chrono::milliseconds{10}, resolver));
    static_cast<void>(model.update(makeActiveResult(1, 0), openedAt + std::chrono::milliseconds{20}, resolver));
    require(model.getLyrics() == "Revision 1", "first revision must resolve its own lyric text");

    static_cast<void>(model.update(makeActiveResult(2, 0), openedAt + std::chrono::milliseconds{30}, resolver));
    require(model.getActiveCue() == ActiveCueReference{2, 0} && model.getLyrics() == "Revision 2",
            "slot reuse with the same index must still resolve by revision");

    static_cast<void>(model.update(makeActiveResult(3, 0), openedAt + std::chrono::milliseconds{40}, resolver));
    require(model.getMode() == LyricsDisplayMode::cueUnavailable && model.getLyrics().empty(),
            "an unavailable revision must clear old lyrics");
    const auto callsAfterMissingRevision = resolverCalls;
    require(!model.update(makeActiveResult(3, 0), openedAt + std::chrono::milliseconds{45}, resolver) &&
                resolverCalls == callsAfterMissingRevision,
            "an unchanged unavailable revision must not be resolved or rendered repeatedly");

    static_cast<void>(model.update(makeActiveResult(4, 7), openedAt + std::chrono::milliseconds{50}, resolver));
    require(model.getMode() == LyricsDisplayMode::cueUnavailable && model.getLyrics().empty(),
            "an out-of-range cue index must clear old lyrics");
}

void testEditorReopeningRequiresFreshConfirmation()
{
    const LyricsDisplayModel::TimePoint openedAt{};
    const auto resolver = [](ActiveCueReference) -> std::optional<std::string> { return "Lyrics"; };

    LyricsDisplayModel firstEditor(openedAt);
    static_cast<void>(firstEditor.update(makeActiveResult(1, 0), openedAt + std::chrono::milliseconds{10}, resolver));
    static_cast<void>(firstEditor.update(makeActiveResult(1, 0), openedAt + std::chrono::milliseconds{20}, resolver));
    require(firstEditor.getMode() == LyricsDisplayMode::activeCue, "the first editor must reach an active state");

    LyricsDisplayModel reopenedEditor(openedAt + std::chrono::milliseconds{30});
    static_cast<void>(
        reopenedEditor.update(makeActiveResult(1, 0), openedAt + std::chrono::milliseconds{40}, resolver));
    require(reopenedEditor.getMode() == LyricsDisplayMode::staleSnapshot && reopenedEditor.getLyrics().empty(),
            "a reopened editor must not trust one potentially queued active snapshot");

    static_cast<void>(
        reopenedEditor.update(makeActiveResult(1, 0), openedAt + std::chrono::milliseconds{50}, resolver));
    require(reopenedEditor.getMode() == LyricsDisplayMode::activeCue && reopenedEditor.getLyrics() == "Lyrics",
            "a subsequent fresh snapshot must restore lyrics after reopening");
}
} // namespace

int main()
{
    testActiveCueResolutionAndUnchangedUpdates();
    testStoppedInvalidHostAndNoCueStates();
    testStaleSnapshotIsDistinctFromStopped();
    testOverflowAndAudioLifecycleInvalidation();
    testRevisionChangesAndInvalidCueReferences();
    testEditorReopeningRequiresFreshConfirmation();

    std::cout << "ShowLyricsLyricsDisplayModelTests: PASS\n";
    return EXIT_SUCCESS;
}
