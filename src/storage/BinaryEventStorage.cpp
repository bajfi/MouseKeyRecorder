#include "BinaryEventStorage.hpp"
#include "core/Event.hpp"
#include <fstream>
#include <spdlog/spdlog.h>
#include <cstring>

namespace MouseRecorder::Storage
{

BinaryEventStorage::BinaryEventStorage()
{
    spdlog::debug("BinaryEventStorage: Constructor");
}

bool BinaryEventStorage::saveEvents(
  const std::vector<std::unique_ptr<Core::Event>>& events,
  const std::string& filename,
  const Core::StorageMetadata& metadata
)
{
    spdlog::info(
      "BinaryEventStorage: Saving {} events to {}", events.size(), filename
    );

    try
    {
        std::vector<uint8_t> buffer;

        // Write header
        writeBinary(buffer, MAGIC_NUMBER);
        writeBinary(buffer, FORMAT_VERSION);

        // Serialize metadata
        std::vector<uint8_t> metadataBuffer;
        serializeMetadata(metadata, metadataBuffer);
        writeBinary(buffer, static_cast<uint32_t>(metadataBuffer.size()));
        buffer.insert(
          buffer.end(), metadataBuffer.begin(), metadataBuffer.end()
        );

        // Write event count
        writeBinary(buffer, static_cast<uint32_t>(events.size()));

        // Serialize events
        for (const auto& event : events)
        {
            if (event)
            {
                serializeEvent(*event, buffer);
            }
        }

        // Apply compression if enabled
        std::vector<uint8_t> finalData;
        if (m_compressionEnabled)
        {
            finalData = compressData(buffer);
        }
        else
        {
            finalData = std::move(buffer);
        }

        // Write to file
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open())
        {
            setLastError("Failed to open file for writing: " + filename);
            return false;
        }

        file.write(
          reinterpret_cast<const char*>(finalData.data()), finalData.size()
        );
        file.close();

        if (file.fail())
        {
            setLastError("Failed to write binary data to file");
            return false;
        }

        spdlog::info(
          "BinaryEventStorage: Successfully saved {} events ({} bytes)",
          events.size(),
          finalData.size()
        );
        return true;
    }
    catch (const std::exception& e)
    {
        setLastError("Binary serialization error: " + std::string(e.what()));
        return false;
    }
}

bool BinaryEventStorage::loadEvents(
  const std::string& filename,
  std::vector<std::unique_ptr<Core::Event>>& events,
  Core::StorageMetadata& metadata
)
{
    spdlog::info("BinaryEventStorage: Loading events from {}", filename);

    try
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open())
        {
            setLastError("Failed to open file for reading: " + filename);
            return false;
        }

        // Read entire file
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> fileData(fileSize);
        file.read(reinterpret_cast<char*>(fileData.data()), fileSize);
        file.close();

        // Decompress if needed
        std::vector<uint8_t> buffer;
        if (m_compressionEnabled)
        {
            buffer = decompressData(fileData);
        }
        else
        {
            buffer = std::move(fileData);
        }

        size_t offset = 0;

        // Read and validate header
        uint32_t magic = readBinary<uint32_t>(buffer, offset);
        if (magic != MAGIC_NUMBER)
        {
            setLastError("Invalid file format: magic number mismatch");
            return false;
        }

        uint32_t version = readBinary<uint32_t>(buffer, offset);
        if (version != FORMAT_VERSION)
        {
            setLastError(
              "Unsupported file version: " + std::to_string(version)
            );
            return false;
        }

        // Read metadata
        uint32_t metadataSize = readBinary<uint32_t>(buffer, offset);
        if (offset + metadataSize > buffer.size())
        {
            setLastError("Corrupted file: metadata size exceeds file size");
            return false;
        }

        metadata = deserializeMetadata(buffer, offset);

        // Read event count
        uint32_t eventCount = readBinary<uint32_t>(buffer, offset);

        // Read events
        events.clear();
        events.reserve(eventCount);

        for (uint32_t i = 0; i < eventCount; ++i)
        {
            auto event = deserializeEvent(buffer, offset);
            if (event)
            {
                events.push_back(std::move(event));
            }
            else
            {
                spdlog::warn(
                  "BinaryEventStorage: Failed to deserialize event {}", i
                );
            }
        }

        spdlog::info(
          "BinaryEventStorage: Successfully loaded {} events", events.size()
        );
        return true;
    }
    catch (const std::exception& e)
    {
        setLastError("Binary deserialization error: " + std::string(e.what()));
        return false;
    }
}

Core::StorageFormat BinaryEventStorage::getSupportedFormat() const noexcept
{
    return Core::StorageFormat::Binary;
}

std::string BinaryEventStorage::getFileExtension() const noexcept
{
    return ".mre"; // MouseRecorder Events
}

std::string BinaryEventStorage::getFormatDescription() const noexcept
{
    return "Binary Event Recording";
}

bool BinaryEventStorage::validateFile(const std::string& filename) const
{
    try
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open())
        {
            return false;
        }

        // Read and validate header only
        uint32_t magic, version;
        file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        file.read(reinterpret_cast<char*>(&version), sizeof(version));

        return magic == MAGIC_NUMBER && version == FORMAT_VERSION;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool BinaryEventStorage::getFileMetadata(
  const std::string& filename, Core::StorageMetadata& metadata
) const
{
    try
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open())
        {
            return false;
        }

        std::vector<uint8_t> buffer;

        // Read header
        uint32_t magic, version;
        file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        file.read(reinterpret_cast<char*>(&version), sizeof(version));

        if (magic != MAGIC_NUMBER || version != FORMAT_VERSION)
        {
            return false;
        }

        // Read metadata size
        uint32_t metadataSize;
        file.read(reinterpret_cast<char*>(&metadataSize), sizeof(metadataSize));

        // Read metadata
        buffer.resize(metadataSize);
        file.read(reinterpret_cast<char*>(buffer.data()), metadataSize);

        size_t offset = 0;
        metadata = deserializeMetadata(buffer, offset);

        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

std::string BinaryEventStorage::getLastError() const
{
    return m_lastError;
}

void BinaryEventStorage::setCompressionLevel(int level)
{
    m_compressionEnabled = (level > 0);
    spdlog::debug(
      "BinaryEventStorage: Compression set to {}", m_compressionEnabled
    );
}

bool BinaryEventStorage::supportsCompression() const noexcept
{
    return true;
}

void BinaryEventStorage::serializeEvent(
  const Core::Event& event, std::vector<uint8_t>& buffer
) const
{
    // Event type
    writeBinary(buffer, static_cast<uint8_t>(event.getType()));

    // Timestamp
    writeBinary(buffer, event.getTimestampMs());

    // Event data based on type
    switch (event.getType())
    {
    case Core::EventType::MouseMove:
    case Core::EventType::MouseClick:
    case Core::EventType::MouseDoubleClick:
    case Core::EventType::MouseWheel: {
        const auto* mouseData = event.getMouseData();
        writeBinary(buffer, static_cast<int32_t>(mouseData->position.x));
        writeBinary(buffer, static_cast<int32_t>(mouseData->position.y));
        writeBinary(buffer, static_cast<uint8_t>(mouseData->button));
        writeBinary(buffer, static_cast<int32_t>(mouseData->wheelDelta));
        writeBinary(buffer, static_cast<uint32_t>(mouseData->modifiers));
        break;
    }

    case Core::EventType::KeyPress:
    case Core::EventType::KeyRelease:
    case Core::EventType::KeyCombination: {
        const auto* keyData = event.getKeyboardData();
        writeBinary(buffer, keyData->keyCode);
        writeString(buffer, keyData->keyName);
        writeBinary(buffer, static_cast<uint32_t>(keyData->modifiers));
        writeBinary(buffer, static_cast<uint8_t>(keyData->isRepeated ? 1 : 0));
        break;
    }
    }
}

std::unique_ptr<Core::Event> BinaryEventStorage::deserializeEvent(
  const std::vector<uint8_t>& buffer, size_t& offset
) const
{
    try
    {
        // Event type
        auto eventType =
          static_cast<Core::EventType>(readBinary<uint8_t>(buffer, offset));

        // Timestamp
        uint64_t timestamp = readBinary<uint64_t>(buffer, offset);
        auto timePoint = Core::Event::timestampFromMs(timestamp);

        // Event data based on type
        switch (eventType)
        {
        case Core::EventType::MouseMove:
        case Core::EventType::MouseClick:
        case Core::EventType::MouseDoubleClick:
        case Core::EventType::MouseWheel: {
            Core::MouseEventData mouseData;
            mouseData.position.x = readBinary<int32_t>(buffer, offset);
            mouseData.position.y = readBinary<int32_t>(buffer, offset);
            mouseData.button = static_cast<Core::MouseButton>(
              readBinary<uint8_t>(buffer, offset)
            );
            mouseData.wheelDelta = readBinary<int32_t>(buffer, offset);
            mouseData.modifiers = static_cast<Core::KeyModifier>(
              readBinary<uint32_t>(buffer, offset)
            );

            return std::make_unique<Core::Event>(
              eventType, mouseData, timePoint
            );
        }

        case Core::EventType::KeyPress:
        case Core::EventType::KeyRelease:
        case Core::EventType::KeyCombination: {
            Core::KeyboardEventData keyData;
            keyData.keyCode = readBinary<uint32_t>(buffer, offset);
            keyData.keyName = readString(buffer, offset);
            keyData.modifiers = static_cast<Core::KeyModifier>(
              readBinary<uint32_t>(buffer, offset)
            );
            keyData.isRepeated = (readBinary<uint8_t>(buffer, offset) != 0);

            return std::make_unique<Core::Event>(eventType, keyData, timePoint);
        }
        }

        return nullptr;
    }
    catch (const std::exception&)
    {
        return nullptr;
    }
}

void BinaryEventStorage::serializeMetadata(
  const Core::StorageMetadata& metadata, std::vector<uint8_t>& buffer
) const
{
    writeString(buffer, metadata.version);
    writeString(buffer, metadata.applicationName);
    writeString(buffer, metadata.createdBy);
    writeString(buffer, metadata.description);
    writeBinary(buffer, metadata.creationTimestamp);
    writeBinary(buffer, metadata.totalDurationMs);
    writeBinary(buffer, metadata.totalEvents);
    writeString(buffer, metadata.platform);
    writeString(buffer, metadata.screenResolution);
}

Core::StorageMetadata BinaryEventStorage::deserializeMetadata(
  const std::vector<uint8_t>& buffer, size_t& offset
) const
{
    Core::StorageMetadata metadata;

    metadata.version = readString(buffer, offset);
    metadata.applicationName = readString(buffer, offset);
    metadata.createdBy = readString(buffer, offset);
    metadata.description = readString(buffer, offset);
    metadata.creationTimestamp = readBinary<uint64_t>(buffer, offset);
    metadata.totalDurationMs = readBinary<uint64_t>(buffer, offset);
    metadata.totalEvents = readBinary<size_t>(buffer, offset);
    metadata.platform = readString(buffer, offset);
    metadata.screenResolution = readString(buffer, offset);

    return metadata;
}

template <typename T>
void BinaryEventStorage::writeBinary(
  std::vector<uint8_t>& buffer, const T& value
) const
{
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
    buffer.insert(buffer.end(), bytes, bytes + sizeof(T));
}

template <typename T>
T BinaryEventStorage::readBinary(
  const std::vector<uint8_t>& buffer, size_t& offset
) const
{
    if (offset + sizeof(T) > buffer.size())
    {
        throw std::runtime_error("Buffer underrun while reading binary data");
    }

    T value;
    std::memcpy(&value, buffer.data() + offset, sizeof(T));
    offset += sizeof(T);
    return value;
}

void BinaryEventStorage::writeString(
  std::vector<uint8_t>& buffer, const std::string& str
) const
{
    writeBinary(buffer, static_cast<uint32_t>(str.length()));
    buffer.insert(buffer.end(), str.begin(), str.end());
}

std::string BinaryEventStorage::readString(
  const std::vector<uint8_t>& buffer, size_t& offset
) const
{
    uint32_t length = readBinary<uint32_t>(buffer, offset);

    if (offset + length > buffer.size())
    {
        throw std::runtime_error("Buffer underrun while reading string");
    }

    std::string str(
      reinterpret_cast<const char*>(buffer.data() + offset), length
    );
    offset += length;
    return str;
}

std::vector<uint8_t> BinaryEventStorage::compressData(
  const std::vector<uint8_t>& input
) const
{
    // Simple RLE compression
    std::vector<uint8_t> output;

    if (input.empty())
    {
        return output;
    }

    output.push_back(0x01); // Compression flag

    for (size_t i = 0; i < input.size();)
    {
        uint8_t currentByte = input[i];
        size_t count = 1;

        // Count consecutive identical bytes
        while (i + count < input.size() && input[i + count] == currentByte &&
               count < 255)
        {
            ++count;
        }

        if (count > 3 || currentByte == 0x00 || currentByte == 0xFF)
        {
            // Use RLE encoding
            output.push_back(0x00); // RLE marker
            output.push_back(static_cast<uint8_t>(count));
            output.push_back(currentByte);
        }
        else
        {
            // Store literally
            for (size_t j = 0; j < count; ++j)
            {
                output.push_back(currentByte);
            }
        }

        i += count;
    }

    return output;
}

std::vector<uint8_t> BinaryEventStorage::decompressData(
  const std::vector<uint8_t>& input
) const
{
    if (input.empty() || input[0] != 0x01)
    {
        // Not compressed
        return input;
    }

    std::vector<uint8_t> output;

    for (size_t i = 1; i < input.size();)
    {
        if (input[i] == 0x00 && i + 2 < input.size())
        {
            // RLE sequence
            uint8_t count = input[i + 1];
            uint8_t value = input[i + 2];

            for (uint8_t j = 0; j < count; ++j)
            {
                output.push_back(value);
            }

            i += 3;
        }
        else
        {
            // Literal byte
            output.push_back(input[i]);
            ++i;
        }
    }

    return output;
}

void BinaryEventStorage::setLastError(const std::string& error)
{
    m_lastError = error;
    spdlog::error("BinaryEventStorage: {}", error);
}

} // namespace MouseRecorder::Storage
