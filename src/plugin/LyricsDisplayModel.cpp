// SPDX-License-Identifier: AGPL-3.0-only

#include "LyricsDisplayModel.h"

namespace showlyrics
{
LyricsDisplayModel::LyricsDisplayModel(TimePoint openedAt) noexcept : openedAt_(openedAt)
{
}

LyricsDisplayMode LyricsDisplayModel::getMode() const noexcept
{
    return mode_;
}

const std::optional<ActiveCueReference> &LyricsDisplayModel::getActiveCue() const noexcept
{
    return activeCue_;
}

const std::string &LyricsDisplayModel::getLyrics() const noexcept
{
    return lyrics_;
}

bool LyricsDisplayModel::updateTransportState(const TransportSnapshotReadResult &result, TimePoint now) noexcept
{
    if (!result.audioProcessingAvailable)
        return setMode(LyricsDisplayMode::audioUnavailable);

    if (result.overflowed)
        return setMode(LyricsDisplayMode::mailboxOverflow);

    if (!result.hasSnapshot)
    {
        if (isStale(now))
            return setMode(LyricsDisplayMode::staleSnapshot);
        return false;
    }

    const auto isFirstSnapshot = !hasReceivedSnapshot_;
    hasReceivedSnapshot_ = true;
    lastSnapshotAt_ = now;

    const auto &snapshot = result.snapshot;
    if (!snapshot.has(TransportSnapshotField::transportState))
        return setMode(LyricsDisplayMode::invalidHostPosition);

    if (!snapshot.is(TransportSnapshotState::playing))
        return setMode(LyricsDisplayMode::stopped);

    if (!snapshot.has(TransportSnapshotField::ppqPosition))
        return setMode(LyricsDisplayMode::invalidHostPosition);

    if (!snapshot.has(TransportSnapshotField::activeCue))
        return setMode(LyricsDisplayMode::noCue);

    if (snapshot.cueRevision == 0)
        return setMode(LyricsDisplayMode::cueUnavailable);

    // The first active snapshot may have waited in the bounded mailbox while
    // the editor was closed. A second newly consumed snapshot is required
    // before any lyric text becomes visible.
    if (isFirstSnapshot)
        return setMode(LyricsDisplayMode::staleSnapshot);

    return setActiveCue({snapshot.cueRevision, snapshot.activeCueIndex});
}

bool LyricsDisplayModel::setMode(LyricsDisplayMode mode) noexcept
{
    const auto changed = mode_ != mode || activeCue_.has_value();
    mode_ = mode;
    activeCue_.reset();
    return changed;
}

bool LyricsDisplayModel::setActiveCue(ActiveCueReference cue) noexcept
{
    if ((mode_ == LyricsDisplayMode::activeCue || mode_ == LyricsDisplayMode::cueUnavailable) && activeCue_ &&
        *activeCue_ == cue)
        return false;

    mode_ = LyricsDisplayMode::activeCue;
    activeCue_ = cue;
    return true;
}

bool LyricsDisplayModel::isStale(TimePoint now) const noexcept
{
    const auto referenceTime = lastSnapshotAt_.value_or(openedAt_);
    return now >= referenceTime && now - referenceTime >= staleSnapshotTimeout;
}
} // namespace showlyrics
