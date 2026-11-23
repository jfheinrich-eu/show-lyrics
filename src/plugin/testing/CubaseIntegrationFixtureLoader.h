// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <string_view>

class ShowLyricsAudioProcessor;

namespace showlyrics::testing
{
enum class CubaseIntegrationFixtureLoadResult
{
    loaded,
    malformedJson,
    invalidFixture,
    invalidCueState,
    serializationFailed
};

// Test-build only. The JSON is converted to the validated binary state format
// and then loaded exclusively through AudioProcessor::setStateInformation().
[[nodiscard]] CubaseIntegrationFixtureLoadResult loadCubaseIntegrationFixture(ShowLyricsAudioProcessor &processor,
                                                                              std::string_view fixtureJson);
} // namespace showlyrics::testing
