// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "model/CueGenerationExchange.h"
#include "model/TransportSnapshot.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace showlyrics
{
[[nodiscard]] TransportSnapshot makeTransportSnapshot(const juce::AudioPlayHead::PositionInfo *position,
                                                      const CueGenerationAudioView &generation) noexcept;
} // namespace showlyrics
