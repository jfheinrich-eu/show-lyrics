// SPDX-License-Identifier: AGPL-3.0-only

#include "CubaseIntegrationFixtureLoader.h"

#include "model/CueState.h"
#include "model/CueStateSerializer.h"
#include "model/LyricCue.h"
#include "plugin/PluginProcessor.h"

#include <juce_core/juce_core.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace showlyrics::testing
{
class CubaseIntegrationFixtureLoaderAccess final
{
  public:
    [[nodiscard]] static std::optional<CueGenerationPublishResult> restore(ShowLyricsAudioProcessor &processor,
                                                                           const void *data, int sizeInBytes)
    {
        return processor.restoreCueStateInformation(data, sizeInBytes);
    }
};

namespace
{
constexpr std::size_t maximumFixtureBytes = 64U * 1024U;

bool hasExactProperties(const juce::DynamicObject &object, std::initializer_list<const char *> names)
{
    const auto &properties = object.getProperties();
    if (properties.size() != static_cast<int>(names.size()))
        return false;

    for (const auto *name : names)
    {
        if (!properties.contains(juce::Identifier{name}))
            return false;
    }
    return true;
}

std::optional<int> readInteger(const juce::var &value) noexcept
{
    if (!value.isInt())
        return std::nullopt;
    return static_cast<int>(value);
}

std::optional<double> readNumber(const juce::var &value) noexcept
{
    double number = 0.0;
    if (value.isInt())
        number = static_cast<double>(static_cast<int>(value));
    else if (value.isInt64())
        number = static_cast<double>(static_cast<juce::int64>(value));
    else if (value.isDouble())
        number = static_cast<double>(value);
    else
        return std::nullopt;

    if (!std::isfinite(number))
        return std::nullopt;
    return number;
}

std::optional<std::string> readString(const juce::var &value)
{
    if (!value.isString())
        return std::nullopt;
    return value.toString().toStdString();
}
} // namespace

CubaseIntegrationFixtureLoadResult loadCubaseIntegrationFixture(ShowLyricsAudioProcessor &processor,
                                                                std::string_view fixtureJson)
{
    if (fixtureJson.empty() || fixtureJson.size() > maximumFixtureBytes ||
        fixtureJson.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        return CubaseIntegrationFixtureLoadResult::malformedJson;
    }

    juce::var parsed;
    const auto parseResult =
        juce::JSON::parse(juce::String::fromUTF8(fixtureJson.data(), static_cast<int>(fixtureJson.size())), parsed);
    if (parseResult.failed())
        return CubaseIntegrationFixtureLoadResult::malformedJson;

    const auto *root = parsed.getDynamicObject();
    if (root == nullptr || !hasExactProperties(*root, {"fixtureVersion", "tempoBpm", "timeSignature", "cues"}))
        return CubaseIntegrationFixtureLoadResult::invalidFixture;

    const auto fixtureVersion = readInteger(root->getProperty("fixtureVersion"));
    const auto tempo = readNumber(root->getProperty("tempoBpm"));
    const auto *timeSignature = root->getProperty("timeSignature").getDynamicObject();
    if (!fixtureVersion || *fixtureVersion != 1 || !tempo || *tempo != 120.0 || timeSignature == nullptr ||
        !hasExactProperties(*timeSignature, {"numerator", "denominator"}))
    {
        return CubaseIntegrationFixtureLoadResult::invalidFixture;
    }

    const auto numerator = readInteger(timeSignature->getProperty("numerator"));
    const auto denominator = readInteger(timeSignature->getProperty("denominator"));
    const auto *cueArray = root->getProperty("cues").getArray();
    if (!numerator || *numerator != 4 || !denominator || *denominator != 4 || cueArray == nullptr ||
        cueArray->size() != 4)
    {
        return CubaseIntegrationFixtureLoadResult::invalidFixture;
    }

    std::vector<LyricCue> cues;
    cues.reserve(static_cast<std::size_t>(cueArray->size()));
    for (const auto &value : *cueArray)
    {
        const auto *cue = value.getDynamicObject();
        if (cue == nullptr || !hasExactProperties(*cue, {"id", "startQuarterNotes", "endQuarterNotes", "title",
                                                         "lyrics", "displayOrder"}))
        {
            return CubaseIntegrationFixtureLoadResult::invalidFixture;
        }

        const auto identifier = readString(cue->getProperty("id"));
        const auto start = readNumber(cue->getProperty("startQuarterNotes"));
        const auto end = readNumber(cue->getProperty("endQuarterNotes"));
        const auto title = readString(cue->getProperty("title"));
        const auto lyrics = readString(cue->getProperty("lyrics"));
        const auto displayOrder = readInteger(cue->getProperty("displayOrder"));
        if (!identifier || !start || !end || !title || !lyrics || !displayOrder || *displayOrder < 0)
            return CubaseIntegrationFixtureLoadResult::invalidFixture;

        cues.emplace_back(*identifier, *start, *end, *title, *lyrics, static_cast<std::uint32_t>(*displayOrder));
    }

    CueStateValidationResult validation;
    auto state = CueState::create(std::move(cues), validation);
    if (!state)
        return CubaseIntegrationFixtureLoadResult::invalidCueState;

    const auto serialized = CueStateSerializer::serialize(*state);
    if (!serialized)
        return CubaseIntegrationFixtureLoadResult::serializationFailed;

    const auto publication = CubaseIntegrationFixtureLoaderAccess::restore(processor, serialized.data.data(),
                                                                           static_cast<int>(serialized.data.size()));
    if (!publication)
        return CubaseIntegrationFixtureLoadResult::publicationFailed;
    if (*publication == CueGenerationPublishResult::published)
        return CubaseIntegrationFixtureLoadResult::loaded;
    if (*publication == CueGenerationPublishResult::slotsFull)
        return CubaseIntegrationFixtureLoadResult::generationSlotsFull;
    return CubaseIntegrationFixtureLoadResult::publicationFailed;
}
} // namespace showlyrics::testing
