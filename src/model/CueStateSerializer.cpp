// SPDX-License-Identifier: AGPL-3.0-only

#include "CueStateSerializer.h"

#include "CueStateLimits.h"

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
const juce::Identifier rootType{"ShowLyricsState"};
const juce::Identifier cueType{"Cue"};
const juce::Identifier schemaVersionProperty{"schemaVersion"};
const juce::Identifier identifierProperty{"id"};
const juce::Identifier startProperty{"startQuarterNotes"};
const juce::Identifier endProperty{"endQuarterNotes"};
const juce::Identifier titleProperty{"title"};
const juce::Identifier lyricsProperty{"lyrics"};
const juce::Identifier displayOrderProperty{"displayOrder"};
constexpr std::array<std::uint8_t, 4> stateMagic{'S', 'L', 'S', 'T'};
constexpr std::size_t envelopeSize = stateMagic.size() + sizeof(std::uint32_t);

bool hasExactCueProperties(const juce::ValueTree &cue) noexcept
{
    return cue.getNumProperties() == 6 && cue.hasProperty(identifierProperty) && cue.hasProperty(startProperty) &&
           cue.hasProperty(endProperty) && cue.hasProperty(titleProperty) && cue.hasProperty(lyricsProperty) &&
           cue.hasProperty(displayOrderProperty);
}

void appendPayloadSize(std::vector<std::uint8_t> &data, std::uint32_t payloadSize)
{
    for (unsigned int shift = 0; shift < 32; shift += 8)
        data.push_back(static_cast<std::uint8_t>((payloadSize >> shift) & 0xffU));
}

std::uint32_t readPayloadSize(const std::uint8_t *data) noexcept
{
    std::uint32_t payloadSize = 0;
    for (unsigned int shift = 0; shift < 32; shift += 8)
        payloadSize |= static_cast<std::uint32_t>(data[shift / 8]) << shift;
    return payloadSize;
}

class BoundedReader final
{
  public:
    BoundedReader(const std::uint8_t *data, std::size_t size) noexcept : current_(data), end_(data + size)
    {
    }

    [[nodiscard]] std::optional<std::string_view> readCString() noexcept
    {
        const auto *begin = current_;
        while (current_ != end_ && *current_ != 0)
            ++current_;
        if (current_ == end_)
            return std::nullopt;

        const auto size = static_cast<std::size_t>(current_ - begin);
        ++current_;
        return std::string_view(reinterpret_cast<const char *>(begin), size);
    }

    [[nodiscard]] std::optional<int> readCompressedInt() noexcept
    {
        const auto sizeByte = readByte();
        if (!sizeByte)
            return std::nullopt;
        if (*sizeByte == 0)
            return 0;

        const auto numberOfBytes = static_cast<std::size_t>(*sizeByte & 0x7fU);
        if (numberOfBytes == 0 || numberOfBytes > 4 || remaining() < numberOfBytes)
            return std::nullopt;

        std::uint32_t value = 0;
        for (std::size_t index = 0; index < numberOfBytes; ++index)
            value |= static_cast<std::uint32_t>(current_[index]) << (index * 8U);
        current_ += numberOfBytes;

        if ((*sizeByte & 0x80U) != 0 || value > static_cast<std::uint32_t>(std::numeric_limits<int>::max()))
            return std::nullopt;
        return static_cast<int>(value);
    }

    [[nodiscard]] bool readIntegerVariant() noexcept
    {
        return readVariantHeader(5, 1) && skip(4);
    }

    [[nodiscard]] bool readDoubleVariant() noexcept
    {
        return readVariantHeader(9, 4) && skip(8);
    }

    [[nodiscard]] bool readStringVariant(std::size_t maximumBytes) noexcept
    {
        const auto variantSize = readCompressedInt();
        if (!variantSize || *variantSize < 2 || static_cast<std::size_t>(*variantSize) > maximumBytes + 2)
            return false;

        const auto marker = readByte();
        const auto stringSize = static_cast<std::size_t>(*variantSize - 1);
        if (!marker || *marker != 5 || remaining() < stringSize || current_[stringSize - 1] != 0)
            return false;
        if (std::find(current_, current_ + stringSize - 1, 0) != current_ + stringSize - 1)
            return false;

        current_ += stringSize;
        return true;
    }

    [[nodiscard]] bool finished() const noexcept
    {
        return current_ == end_;
    }

  private:
    [[nodiscard]] std::optional<std::uint8_t> readByte() noexcept
    {
        if (current_ == end_)
            return std::nullopt;
        return *current_++;
    }

    [[nodiscard]] bool readVariantHeader(int expectedSize, std::uint8_t expectedMarker) noexcept
    {
        const auto size = readCompressedInt();
        const auto marker = readByte();
        return size && *size == expectedSize && marker && *marker == expectedMarker;
    }

    [[nodiscard]] bool skip(std::size_t numberOfBytes) noexcept
    {
        if (remaining() < numberOfBytes)
            return false;
        current_ += numberOfBytes;
        return true;
    }

    [[nodiscard]] std::size_t remaining() const noexcept
    {
        return static_cast<std::size_t>(end_ - current_);
    }

    const std::uint8_t *current_;
    const std::uint8_t *end_;
};

bool readCueProperties(BoundedReader &reader) noexcept
{
    unsigned int propertiesSeen = 0;
    for (int propertyIndex = 0; propertyIndex < 6; ++propertyIndex)
    {
        const auto name = reader.readCString();
        if (!name)
            return false;

        unsigned int propertyBit = 0;
        bool validValue = false;
        if (*name == "id")
        {
            propertyBit = 1U << 0U;
            validValue = reader.readStringVariant(36);
        }
        else if (*name == "startQuarterNotes")
        {
            propertyBit = 1U << 1U;
            validValue = reader.readDoubleVariant();
        }
        else if (*name == "endQuarterNotes")
        {
            propertyBit = 1U << 2U;
            validValue = reader.readDoubleVariant();
        }
        else if (*name == "title")
        {
            propertyBit = 1U << 3U;
            validValue = reader.readStringVariant(showlyrics::limits::maximumTitleBytes);
        }
        else if (*name == "lyrics")
        {
            propertyBit = 1U << 4U;
            validValue = reader.readStringVariant(showlyrics::limits::maximumLyricsBytes);
        }
        else if (*name == "displayOrder")
        {
            propertyBit = 1U << 5U;
            validValue = reader.readIntegerVariant();
        }

        if (!validValue || (propertiesSeen & propertyBit) != 0)
            return false;
        propertiesSeen |= propertyBit;
    }

    return propertiesSeen == 0x3fU;
}

bool isSafeValueTreePayload(const std::uint8_t *data, std::size_t size) noexcept
{
    BoundedReader reader(data, size);
    const auto rootName = reader.readCString();
    const auto rootPropertyCount = reader.readCompressedInt();
    const auto versionName = reader.readCString();
    if (!rootName || *rootName != "ShowLyricsState" || !rootPropertyCount || *rootPropertyCount != 1 || !versionName ||
        *versionName != "schemaVersion" || !reader.readIntegerVariant())
        return false;

    const auto cueCount = reader.readCompressedInt();
    if (!cueCount || *cueCount < 0 || static_cast<std::size_t>(*cueCount) > showlyrics::limits::maximumCueCount)
        return false;

    for (int cueIndex = 0; cueIndex < *cueCount; ++cueIndex)
    {
        const auto cueName = reader.readCString();
        const auto propertyCount = reader.readCompressedInt();
        if (!cueName || *cueName != "Cue" || !propertyCount || *propertyCount != 6 || !readCueProperties(reader))
            return false;

        const auto childCount = reader.readCompressedInt();
        if (!childCount || *childCount != 0)
            return false;
    }

    return reader.finished();
}
} // namespace

namespace showlyrics
{
CueStateSerializationResult CueStateSerializer::serialize(const CueState &state)
{
    juce::ValueTree root(rootType);
    root.setProperty(schemaVersionProperty, schemaVersion, nullptr);

    for (const auto &cue : state.getCues())
    {
        juce::ValueTree serializedCue(cueType);
        serializedCue.setProperty(identifierProperty, juce::String::fromUTF8(cue.getIdentifier().c_str()), nullptr);
        serializedCue.setProperty(startProperty, cue.getStartQuarterNotes(), nullptr);
        serializedCue.setProperty(endProperty, cue.getEndQuarterNotes(), nullptr);
        serializedCue.setProperty(
            titleProperty, juce::String::fromUTF8(cue.getTitle().data(), static_cast<int>(cue.getTitle().size())),
            nullptr);
        serializedCue.setProperty(
            lyricsProperty, juce::String::fromUTF8(cue.getLyrics().data(), static_cast<int>(cue.getLyrics().size())),
            nullptr);
        serializedCue.setProperty(displayOrderProperty, static_cast<int>(cue.getDisplayOrder()), nullptr);
        root.appendChild(serializedCue, nullptr);
    }

    juce::MemoryBlock payload;
    juce::MemoryOutputStream output(payload, false);
    root.writeToStream(output);
    output.flush();

    if (payload.getSize() > limits::maximumSerializedStateBytes - envelopeSize)
        return {CueStateError::serializedStateTooLarge, {}};

    std::vector<std::uint8_t> data;
    data.reserve(envelopeSize + payload.getSize());
    data.insert(data.end(), stateMagic.begin(), stateMagic.end());
    appendPayloadSize(data, static_cast<std::uint32_t>(payload.getSize()));
    const auto *payloadBegin = static_cast<const std::uint8_t *>(payload.getData());
    data.insert(data.end(), payloadBegin, payloadBegin + payload.getSize());
    return {CueStateError::none, std::move(data)};
}

CueStateDeserializationResult CueStateSerializer::deserialize(const void *data, std::size_t sizeInBytes)
{
    if (data == nullptr || sizeInBytes < envelopeSize)
        return {CueStateError::malformedState};
    if (sizeInBytes > limits::maximumSerializedStateBytes)
        return {CueStateError::serializedStateTooLarge};

    const auto *bytes = static_cast<const std::uint8_t *>(data);
    if (!std::equal(stateMagic.begin(), stateMagic.end(), bytes))
        return {CueStateError::malformedState};

    const auto payloadSize = static_cast<std::size_t>(readPayloadSize(bytes + stateMagic.size()));
    if (payloadSize != sizeInBytes - envelopeSize)
        return {CueStateError::malformedState};
    if (!isSafeValueTreePayload(bytes + envelopeSize, payloadSize))
        return {CueStateError::malformedState};

    juce::MemoryInputStream input(bytes + envelopeSize, payloadSize, false);
    const auto root = juce::ValueTree::readFromStream(input);
    if (!root.isValid() || !root.hasType(rootType) || input.getPosition() != static_cast<juce::int64>(payloadSize) ||
        root.getNumProperties() != 1 || !root.hasProperty(schemaVersionProperty))
        return {CueStateError::malformedState};

    const auto version = root.getProperty(schemaVersionProperty);
    if (!version.isInt() || static_cast<int>(version) != schemaVersion)
        return {CueStateError::unsupportedSchemaVersion};
    if (root.getNumChildren() > static_cast<int>(limits::maximumCueCount))
        return {CueStateError::tooManyCues};

    std::vector<LyricCue> cues;
    cues.reserve(static_cast<std::size_t>(root.getNumChildren()));

    for (int index = 0; index < root.getNumChildren(); ++index)
    {
        const auto cue = root.getChild(index);
        if (!cue.hasType(cueType) || !hasExactCueProperties(cue))
            return {CueStateError::malformedState};

        const auto identifier = cue.getProperty(identifierProperty);
        const auto start = cue.getProperty(startProperty);
        const auto end = cue.getProperty(endProperty);
        const auto title = cue.getProperty(titleProperty);
        const auto lyrics = cue.getProperty(lyricsProperty);
        const auto displayOrder = cue.getProperty(displayOrderProperty);

        if (!identifier.isString() || !start.isDouble() || !end.isDouble() || !title.isString() || !lyrics.isString() ||
            !displayOrder.isInt())
            return {CueStateError::malformedState};

        const auto order = static_cast<int>(displayOrder);
        if (order < 0)
            return {CueStateError::malformedState};

        cues.emplace_back(identifier.toString().toStdString(), static_cast<double>(start), static_cast<double>(end),
                          title.toString().toStdString(), lyrics.toString().toStdString(),
                          static_cast<std::uint32_t>(order));
    }

    CueStateValidationResult validation;
    auto state = CueState::create(std::move(cues), validation);
    if (!state)
        return {validation.error, validation, std::nullopt};

    return {CueStateError::none, validation, std::move(state)};
}
} // namespace showlyrics
