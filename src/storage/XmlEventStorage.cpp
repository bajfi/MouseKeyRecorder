// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "XmlEventStorage.hpp"
#include "core/Event.hpp"
#include "core/serialization/EventSerializerFactory.hpp"
#include <fstream>
#include "core/SpdlogConfig.hpp"

namespace MouseRecorder::Storage
{

XmlEventStorage::XmlEventStorage()
{
    spdlog::debug("XmlEventStorage: Constructor with default XML serializer");
    try
    {
        m_serializer =
            Core::Serialization::EventSerializerFactory::createSerializer(
                Core::Serialization::SerializationFormat::Xml);
        if (!m_serializer)
        {
            spdlog::error("XmlEventStorage: Failed to create XML serializer");
        }
    }
    catch (const std::exception& e)
    {
        spdlog::error("XmlEventStorage: Exception creating serializer: {}",
                      e.what());
        m_serializer = nullptr;
    }
}

XmlEventStorage::XmlEventStorage(
    std::unique_ptr<Core::Serialization::IEventSerializer> serializer)
    : m_serializer(std::move(serializer))
{
    spdlog::debug("XmlEventStorage: Constructor with custom serializer");
    if (!m_serializer)
    {
        spdlog::error("XmlEventStorage: Provided serializer is null");
    }
}

bool XmlEventStorage::saveEvents(
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
        spdlog::debug(
            "XmlEventStorage: Saving {} events to {}", events.size(), filename);

        // Serialize events to XML string
        std::string xmlData = m_serializer->serializeEvents(events, metadata);

        // Write to file
        std::ofstream file(filename);
        if (!file.is_open())
        {
            setLastError("Failed to open file for writing: " + filename);
            return false;
        }

        file << xmlData;
        file.close();

        if (file.fail())
        {
            setLastError("Failed to write data to file: " + filename);
            return false;
        }

        spdlog::info("XmlEventStorage: Successfully saved {} events to {}",
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

bool XmlEventStorage::loadEvents(
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
        spdlog::debug("XmlEventStorage: Loading events from {}", filename);

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
            setLastError("Failed to deserialize events from XML");
            return false;
        }

        spdlog::info("XmlEventStorage: Successfully loaded {} events from {}",
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

Core::StorageFormat XmlEventStorage::getSupportedFormat() const noexcept
{
    return Core::StorageFormat::Xml;
}

std::string XmlEventStorage::getFileExtension() const noexcept
{
    return ".xml";
}

std::string XmlEventStorage::getFormatDescription() const noexcept
{
    return "XML Event Recording";
}

bool XmlEventStorage::validateFile(const std::string& filename) const
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

bool XmlEventStorage::getFileMetadata(const std::string& filename,
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

std::string XmlEventStorage::getLastError() const
{
    return m_lastError;
}

void XmlEventStorage::setCompressionLevel(int level)
{
    // XmlEventStorage doesn't support compression directly,
    // but this can be passed to the serializer if needed
    (void) level; // Suppress unused parameter warning
}

bool XmlEventStorage::supportsCompression() const noexcept
{
    return false; // XML itself doesn't support compression
}

void XmlEventStorage::setLastError(const std::string& error)
{
    m_lastError = error;
    spdlog::error("XmlEventStorage: {}", error);
}

} // namespace MouseRecorder::Storage
