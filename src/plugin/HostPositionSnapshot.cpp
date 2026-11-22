// SPDX-License-Identifier: AGPL-3.0-only

#include "HostPositionSnapshot.h"

#include <cmath>
#include <cstdint>
#include <limits>

namespace showlyrics
{
namespace
{
void setField(TransportSnapshot &snapshot, TransportSnapshotField field) noexcept
{
    snapshot.availableFields |= static_cast<std::uint8_t>(field);
}

void setState(TransportSnapshot &snapshot, TransportSnapshotState state, bool enabled) noexcept
{
    if (enabled)
        snapshot.transportState |= static_cast<std::uint8_t>(state);
}

bool isValidTempo(double tempo) noexcept
{
    return std::isfinite(tempo) && tempo > 0.0;
}

bool fitsTimeSignatureComponent(int component) noexcept
{
    return component > 0 && component <= static_cast<int>(std::numeric_limits<std::uint16_t>::max());
}
} // namespace

TransportSnapshot makeTransportSnapshot(const juce::AudioPlayHead::PositionInfo *position,
                                        const CueGenerationAudioView &generation) noexcept
{
    TransportSnapshot snapshot;
    snapshot.cueRevision = generation ? generation.revision : 0;

    if (position == nullptr)
        return snapshot;

    // JUCE exposes these transport flags as bool rather than Optional<bool>.
    // An available PositionInfo therefore represents the strongest transport
    // status the API can provide; false is handled conservatively as inactive.
    setField(snapshot, TransportSnapshotField::transportState);
    setState(snapshot, TransportSnapshotState::playing, position->getIsPlaying());
    setState(snapshot, TransportSnapshotState::recording, position->getIsRecording());
    setState(snapshot, TransportSnapshotState::cycling, position->getIsLooping());

    const auto ppqPosition = position->getPpqPosition();
    if (ppqPosition && std::isfinite(*ppqPosition))
    {
        snapshot.ppqPosition = *ppqPosition;
        setField(snapshot, TransportSnapshotField::ppqPosition);
    }

    const auto barStart = position->getPpqPositionOfLastBarStart();
    if (barStart && std::isfinite(*barStart))
    {
        snapshot.barStartQuarterNotes = *barStart;
        setField(snapshot, TransportSnapshotField::barStartPosition);
    }

    const auto tempo = position->getBpm();
    if (tempo && isValidTempo(*tempo))
    {
        snapshot.tempoBpm = *tempo;
        setField(snapshot, TransportSnapshotField::tempo);
    }

    const auto timeSignature = position->getTimeSignature();
    if (timeSignature && fitsTimeSignatureComponent(timeSignature->numerator) &&
        fitsTimeSignatureComponent(timeSignature->denominator))
    {
        snapshot.timeSignatureNumerator = static_cast<std::uint16_t>(timeSignature->numerator);
        snapshot.timeSignatureDenominator = static_cast<std::uint16_t>(timeSignature->denominator);
        setField(snapshot, TransportSnapshotField::timeSignature);
    }

    const auto loopPoints = position->getLoopPoints();
    if (loopPoints && std::isfinite(loopPoints->ppqStart) && std::isfinite(loopPoints->ppqEnd) &&
        loopPoints->ppqStart < loopPoints->ppqEnd)
    {
        snapshot.cycleStartQuarterNotes = loopPoints->ppqStart;
        snapshot.cycleEndQuarterNotes = loopPoints->ppqEnd;
        setField(snapshot, TransportSnapshotField::cycleRange);
    }

    if (!position->getIsPlaying() || !snapshot.has(TransportSnapshotField::ppqPosition) || !generation)
        return snapshot;

    const auto cueIndex = generation.playbackIndex->findCueIndex(snapshot.ppqPosition);
    if (cueIndex)
    {
        snapshot.activeCueIndex = *cueIndex;
        setField(snapshot, TransportSnapshotField::activeCue);
    }

    return snapshot;
}
} // namespace showlyrics
