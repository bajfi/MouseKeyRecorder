// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "JsonEventStorage.hpp"
#include "core/Event.hpp"
#include "core/serialization/EventSerializerFactory.hpp"
#include <fstream>
#include "core/SpdlogConfig.hpp"

namespace MouseRecorder::Storage
{

JsonEventStorage::JsonEventStorage()
{
    spdlog::debug("JsonEventStorage: Constructor with default JSON serializer");
    try
    {
        m_serializer =
            Core::Serialization::EventSerializerFactory::createSerializer(
                Core::Serialization::SerializationFormat::Json);
        if (!m_serializer)
        {
            spdlog::error("JsonEventStorage: Failed to create JSON serializer");
        }
    }
    catch (const std::exception& e)
    {
        spdlog::error("JsonEventStorage: Exception creating serializer: {}",
                      e.what());
        m_serializer = nullptr;
    }
}

JsonEventStorage::JsonEventStorage(
    std::unique_ptr<Core::Serialization::IEventSerializer> serializer)
    : m_serializer(std::move(serializer))
{
    spdlog::debug("JsonEventStorage: Constructor with custom serializer");
    if (!m_serializer)
    {
        spdlog::error("JsonEventStorage: Provided serializer is null");
    }
}

bool JsonEventStorage::saveEvents(
    const std::vector<std::unique_ptr<Core::Event>>& events,
    const std::string& filename,
    const Core::StorageMetadata& metadata)
{
    if (!m_serializer)
    {
        setLastError("No serializer available");
        return false;
    }

    try
    {
        spdlog::debug("JsonEventStorage: Saving {} events to {}",
                      events.size(),
                      filename);

        // Serialize events to JSON string
        std::string jsonData = m_serializer->serializeEvents(events, metadata);

        // Write to file
        std::ofstream file(filename);
        if (!file.is_open())
        {
            setLastError("Failed to open file for writing: " + filename);
            return false;
        }

        file << jsonData;
        file.close();

        if (file.fail())
        {
            setLastError("Failed to write data to file: " + filename);
            return false;
        }

        spdlog::info("JsonEventStorage: Successfully saved {} events to {}",
                     events.size(),
                     filename);
        return true;
    }
    catch (const std::exception& e)
    {
        setLastError("Exception while saving events: " + std::string(e.what()));
        return false;
    }
}

bool JsonEventStorage::loadEvents(
    const std::string& filename,
    std::vector<std::unique_ptr<Core::Event>>& events,
    Core::StorageMetadata& metadata)
{
    if (!m_serializer)
    {
        setLastError("No serializer available");
        return false;
    }

    try
    {
        spdlog::debug("JsonEventStorage: Loading events from {}", filename);

        // Read file content
        std::ifstream file(filename);
        if (!file.is_open())
        {
            setLastError("Failed to open file for reading: " + filename);
            return false;
        }

        std::string fileContent((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
        file.close();

        if (file.fail())
        {
            setLastError("Failed to read file: " + filename);
            return false;
        }

        // Clear existing events
        events.clear();

        // Deserialize events
        if (!m_serializer->deserializeEvents(fileContent, events, metadata))
        {
            setLastError("Failed to deserialize events from JSON");
            return false;
        }

        spdlog::info("JsonEventStorage: Successfully loaded {} events from {}",
                     events.size(),
                     filename);
        return true;
    }
    catch (const std::exception& e)
    {
        setLastError("Exception while loading events: " +
                     std::string(e.what()));
        return false;
    }
}

Core::StorageFormat JsonEventStorage::getSupportedFormat() const noexcept
{
    return Core::StorageFormat::Json;
}

std::string JsonEventStorage::getFileExtension() const noexcept
{
    return ".json";
}

std::string JsonEventStorage::getFormatDescription() const noexcept
{
    return "JSON Event Recording";
}

bool JsonEventStorage::validateFile(const std::string& filename) const
{
    if (!m_serializer)
    {
        return false;
    }

    try
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            return false;
        }

        std::string fileContent((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
        file.close();

        return m_serializer->validateFormat(fileContent);
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool JsonEventStorage::getFileMetadata(const std::string& filename,
                                       Core::StorageMetadata& metadata) const
{
    if (!m_serializer)
    {
        return false;
    }

    try
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            return false;
        }

        // Read entire file into string
        std::string fileContent((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
        file.close();

        // Use a temporary vector to deserialize and get metadata
        std::vector<std::unique_ptr<Core::Event>> tempEvents;
        return m_serializer->deserializeEvents(
            fileContent, tempEvents, metadata);
    }
    catch (const std::exception&)
    {
        return false;
    }
}

std::string JsonEventStorage::getLastError() const
{
    return m_lastError;
}

void JsonEventStorage::setCompressionLevel(int level)
{
    // JsonEventStorage doesn't support compression directly,
    // but this can be passed to the serializer if needed
    (void) level; // Suppress unused parameter warning
}

bool JsonEventStorage::supportsCompression() const noexcept
{
    return false; // JSON itself doesn't support compression
}

void JsonEventStorage::setLastError(const std::string& error)
{
    m_lastError = error;
    spdlog::error("JsonEventStorage: {}", error);
}

} // namespace MouseRecorder::Storage
