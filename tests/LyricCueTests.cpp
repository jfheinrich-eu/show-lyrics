// SPDX-License-Identifier: AGPL-3.0-only

#include "model/CueState.h"
#include "model/CueStateLimits.h"
#include "model/CueStateSerializer.h"
#include "model/LyricCue.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr auto firstIdentifier = "123e4567-e89b-42d3-a456-426614174000";
constexpr auto secondIdentifier = "123e4567-e89b-42d3-b456-426614174001";

void require(bool condition, const char *message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

showlyrics::LyricCue makeCue(std::string identifier = firstIdentifier, double start = 56.0, double end = 88.0,
                             std::string title = "Verse 1", std::string lyrics = "First line\nSecond line",
                             std::uint32_t displayOrder = 0)
{
    return {std::move(identifier), start, end, std::move(title), std::move(lyrics), displayOrder};
}

std::string makeIdentifier(std::size_t value)
{
    std::ostringstream identifier;
    identifier << "123e4567-e89b-42d3-a456-" << std::hex << std::nouppercase << std::setw(12) << std::setfill('0')
               << value;
    return identifier.str();
}

std::vector<showlyrics::LyricCue> makeAdjacentCues(std::size_t count, std::size_t lyricsBytes = 1)
{
    std::vector<showlyrics::LyricCue> cues;
    cues.reserve(count);
    for (std::size_t index = 0; index < count; ++index)
    {
        cues.emplace_back(makeIdentifier(index), static_cast<double>(index), static_cast<double>(index + 1), "",
                          std::string(lyricsBytes, 'x'), static_cast<std::uint32_t>(index));
    }
    return cues;
}

void updateEnvelopePayloadSize(std::vector<std::uint8_t> &data)
{
    const auto payloadSize = static_cast<std::uint32_t>(data.size() - 8);
    for (unsigned int shift = 0; shift < 32; shift += 8)
        data[4 + shift / 8] = static_cast<std::uint8_t>((payloadSize >> shift) & 0xffU);
}

std::vector<std::uint8_t>::iterator findBytes(std::vector<std::uint8_t> &data, const std::string &text)
{
    return std::search(data.begin(), data.end(), text.begin(), text.end());
}

showlyrics::CueState makeState(std::vector<showlyrics::LyricCue> cues)
{
    showlyrics::CueStateValidationResult validation;
    auto state = showlyrics::CueState::create(std::move(cues), validation);
    require(state.has_value(), "test fixture state must be valid");
    return std::move(*state);
}

void testCueValidation()
{
    const auto cue = makeCue();
    require(cue.isValid(), "a valid Unicode cue must pass validation");
    require(cue.contains(56.0), "the start position must be included");
    require(cue.contains(87.999), "positions before the end must be included");
    require(!cue.contains(88.0), "the end position must be excluded");
    require(!cue.contains(std::numeric_limits<double>::quiet_NaN()), "NaN must not match a cue");

    require(makeCue("not-a-uuid").validate() == showlyrics::LyricCueError::invalidIdentifier,
            "identifiers must be canonical version 4 UUIDs");
    require(makeCue(firstIdentifier, 8.0, 4.0).validate() == showlyrics::LyricCueError::invalidRange,
            "the end must be after the start");
    require(makeCue(firstIdentifier, 0.0, 4.0, "", "Text").isValid(), "an empty title must be valid");
    require(makeCue(firstIdentifier, 0.0, 4.0, "Title", "").validate() == showlyrics::LyricCueError::emptyLyrics,
            "lyrics must not be empty");
    require(makeCue(firstIdentifier, 0.0, 4.0, "Title", "Tab\ttext").validate() ==
                showlyrics::LyricCueError::invalidLyricsText,
            "tabs must be rejected");
    require(makeCue(firstIdentifier, 0.0, 4.0, "Title", "Windows\r\nline").validate() ==
                showlyrics::LyricCueError::invalidLyricsText,
            "persisted text must use LF line endings");

    const std::string unicodeLyrics = "Grüße 世界";
    require(makeCue(firstIdentifier, 0.0, 4.0, "", unicodeLyrics).isValid(), "valid UTF-8 text must be accepted");
    const std::string invalidUtf8{"\xc0\xaf", 2};
    require(makeCue(firstIdentifier, 0.0, 4.0, "", invalidUtf8).validate() ==
                showlyrics::LyricCueError::invalidLyricsText,
            "invalid UTF-8 must be rejected");

    require(
        makeCue(firstIdentifier, 0.0, 4.0, std::string(showlyrics::limits::maximumTitleBytes, 'a'), "Text").isValid(),
        "a title at the byte limit must be valid");
    require(makeCue(firstIdentifier, 0.0, 4.0, std::string(showlyrics::limits::maximumTitleBytes + 1, 'a'), "Text")
                    .validate() == showlyrics::LyricCueError::titleTooLarge,
            "an oversized title must be rejected");
    require(makeCue(firstIdentifier, 0.0, 4.0, "", std::string(showlyrics::limits::maximumLyricsBytes, 'a')).isValid(),
            "lyrics at the byte limit must be valid");
    require(makeCue(firstIdentifier, 0.0, 4.0, "", std::string(showlyrics::limits::maximumLyricsBytes + 1, 'a'))
                    .validate() == showlyrics::LyricCueError::lyricsTooLarge,
            "oversized lyrics must be rejected");
}

void testStateValidation()
{
    showlyrics::CueStateValidationResult validation;
    auto adjacent = showlyrics::CueState::create(
        {makeCue(firstIdentifier, 0.0, 4.0, "One", "Alpha", 1), makeCue(secondIdentifier, 4.0, 8.0, "Two", "Beta", 0)},
        validation);
    require(adjacent.has_value(), "adjacent ranges and independent display order must be valid");

    auto overlap = showlyrics::CueState::create(
        {makeCue(firstIdentifier, 0.0, 8.0, "One", "Alpha", 0), makeCue(secondIdentifier, 2.0, 4.0, "Two", "Beta", 1)},
        validation);
    require(!overlap && validation.error == showlyrics::CueStateError::overlappingRanges,
            "a containing range must be detected as an overlap");

    auto duplicateId = showlyrics::CueState::create(
        {makeCue(firstIdentifier, 0.0, 4.0, "One", "Alpha", 0), makeCue(firstIdentifier, 4.0, 8.0, "Two", "Beta", 1)},
        validation);
    require(!duplicateId && validation.error == showlyrics::CueStateError::duplicateIdentifier,
            "duplicate identifiers must be rejected");

    auto duplicateOrder = showlyrics::CueState::create(
        {makeCue(firstIdentifier, 0.0, 4.0, "One", "Alpha", 0), makeCue(secondIdentifier, 4.0, 8.0, "Two", "Beta", 0)},
        validation);
    require(!duplicateOrder && validation.error == showlyrics::CueStateError::invalidDisplayOrder,
            "display order must be unique and contiguous");

    auto maximumCues = showlyrics::CueState::create(makeAdjacentCues(showlyrics::limits::maximumCueCount), validation);
    require(maximumCues.has_value(), "the maximum cue count must be valid");
    auto tooManyCues =
        showlyrics::CueState::create(makeAdjacentCues(showlyrics::limits::maximumCueCount + 1), validation);
    require(!tooManyCues && validation.error == showlyrics::CueStateError::tooManyCues,
            "a cue count above the limit must be rejected before cue validation");

    constexpr auto cuesAtCombinedLimit =
        showlyrics::limits::maximumCombinedTextBytes / showlyrics::limits::maximumLyricsBytes;
    auto maximumText = showlyrics::CueState::create(
        makeAdjacentCues(cuesAtCombinedLimit, showlyrics::limits::maximumLyricsBytes), validation);
    require(maximumText.has_value(), "combined text at the byte limit must be valid");
    auto excessiveTextCues = makeAdjacentCues(cuesAtCombinedLimit, showlyrics::limits::maximumLyricsBytes);
    excessiveTextCues.push_back(makeCue(makeIdentifier(cuesAtCombinedLimit), static_cast<double>(cuesAtCombinedLimit),
                                        static_cast<double>(cuesAtCombinedLimit + 1), "", "x",
                                        static_cast<std::uint32_t>(cuesAtCombinedLimit)));
    auto excessiveText = showlyrics::CueState::create(std::move(excessiveTextCues), validation);
    require(!excessiveText && validation.error == showlyrics::CueStateError::combinedTextTooLarge,
            "combined text above the limit must be rejected");
}

void testSerialization()
{
    const auto original = makeState({makeCue(firstIdentifier, 0.0, 4.0, "", "Grüße 世界", 1),
                                     makeCue(secondIdentifier, 4.0, 12.0, "Section", "First line\nSecond line", 0)});

    const auto serialized = showlyrics::CueStateSerializer::serialize(original);
    require(static_cast<bool>(serialized), "a valid state must serialize");
    require(!serialized.data.empty(), "serialized state must contain data");

    const auto restored = showlyrics::CueStateSerializer::deserialize(serialized.data.data(), serialized.data.size());
    require(static_cast<bool>(restored), "a serialized state must deserialize");
    require(restored.state->getCues().size() == 2, "round-trip must preserve the cue count");
    require(restored.state->getCues()[0].getLyrics() == "Grüße 世界", "round-trip must preserve Unicode text");
    require(restored.state->getCues()[1].getDisplayOrder() == 0, "round-trip must preserve display order");

    auto truncated = serialized.data;
    truncated.pop_back();
    require(showlyrics::CueStateSerializer::deserialize(truncated.data(), truncated.size()).error ==
                showlyrics::CueStateError::malformedState,
            "truncated state must be rejected");

    auto trailing = serialized.data;
    trailing.push_back(0);
    require(showlyrics::CueStateSerializer::deserialize(trailing.data(), trailing.size()).error ==
                showlyrics::CueStateError::malformedState,
            "trailing data must be rejected");

    const std::vector<std::uint8_t> oversized(showlyrics::limits::maximumSerializedStateBytes + 1, 0);
    require(showlyrics::CueStateSerializer::deserialize(oversized.data(), oversized.size()).error ==
                showlyrics::CueStateError::serializedStateTooLarge,
            "oversized state must be rejected before parsing");

    const auto emptySerialized = showlyrics::CueStateSerializer::serialize(makeState({}));
    require(static_cast<bool>(emptySerialized), "an empty state must serialize");
    const auto emptyRestored =
        showlyrics::CueStateSerializer::deserialize(emptySerialized.data.data(), emptySerialized.data.size());
    require(emptyRestored && emptyRestored.state->getCues().empty(), "an empty state must round-trip");

    auto unknownVersion = emptySerialized.data;
    auto versionName = findBytes(unknownVersion, std::string("schemaVersion\0", 14));
    require(versionName != unknownVersion.end(), "test fixture must contain the schema version property");
    const auto versionValue = versionName + 14 + 3;
    require(versionValue < unknownVersion.end(), "test fixture must contain the schema version value");
    *versionValue = 2;
    require(showlyrics::CueStateSerializer::deserialize(unknownVersion.data(), unknownVersion.size()).error ==
                showlyrics::CueStateError::unsupportedSchemaVersion,
            "an unknown future schema version must be rejected");

    auto maliciousCount = emptySerialized.data;
    maliciousCount.pop_back();
    maliciousCount.insert(maliciousCount.end(), {4, 0xff, 0xff, 0xff, 0x7f});
    updateEnvelopePayloadSize(maliciousCount);
    require(showlyrics::CueStateSerializer::deserialize(maliciousCount.data(), maliciousCount.size()).error ==
                showlyrics::CueStateError::malformedState,
            "an unbounded encoded cue count must be rejected before JUCE parsing");

    auto invalidPosition = serialized.data;
    auto startName = findBytes(invalidPosition, std::string("startQuarterNotes\0", 18));
    require(startName != invalidPosition.end(), "test fixture must contain a start position property");
    auto startValue = startName + 18 + 3;
    require(startValue + 8 <= invalidPosition.end(), "test fixture must contain a double start value");
    const std::uint8_t quietNan[] = {0, 0, 0, 0, 0, 0, 0xf8, 0x7f};
    std::copy(std::begin(quietNan), std::end(quietNan), startValue);
    const auto invalidPositionResult =
        showlyrics::CueStateSerializer::deserialize(invalidPosition.data(), invalidPosition.size());
    require(invalidPositionResult.error == showlyrics::CueStateError::invalidCue &&
                invalidPositionResult.validation.cueError == showlyrics::LyricCueError::invalidRange,
            "a non-finite restored position must be rejected by model validation");
}
} // namespace

int main()
{
    testCueValidation();
    testStateValidation();
    testSerialization();

    std::cout << "ShowLyricsDomainTests: PASS\n";
    return EXIT_SUCCESS;
}
