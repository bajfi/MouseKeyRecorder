// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include "core/Event.hpp"
#include "core/IEventStorage.hpp"
#include <memory>
#include <string>
#include <vector>

namespace MouseRecorder::Core::Serialization
{

/**
 * @brief Supported serialization formats
 */
enum class SerializationFormat
{
    Json,
    Xml
};

/**
 * @brief Abstract interface for event serialization and deserialization
 *
 * This interface provides a common abstraction for serializing/deserializing
 * events and metadata to/from different formats (JSON, XML) using different
 * underlying libraries (Qt framework vs third-party libraries).
 *
 * The interface is designed to:
 * - Abstract away the specifics of the underlying serialization library
 * - Allow easy swapping between Qt and third-party implementations
 * - Provide consistent error handling across all implementations
 * - Enable easy testing and mocking of serialization logic
 */
class IEventSerializer
{
  public:
    virtual ~IEventSerializer() = default;

    /**
     * @brief Serialize events and metadata to string format
     * @param events Vector of events to serialize
     * @param metadata Storage metadata to include
     * @param prettyFormat Whether to format output for human readability
     * @return Serialized string or empty string on error
     */
    virtual std::string serializeEvents(
        const std::vector<std::unique_ptr<Event>>& events,
        const StorageMetadata& metadata,
        bool prettyFormat = true) const = 0;

    /**
     * @brief Deserialize events and metadata from string format
     * @param data Serialized string data
     * @param events Output vector for deserialized events
     * @param metadata Output metadata structure
     * @return true on success, false on error
     */
    virtual bool deserializeEvents(const std::string& data,
                                   std::vector<std::unique_ptr<Event>>& events,
                                   StorageMetadata& metadata) const = 0;

    /**
     * @brief Serialize only metadata to string format
     * @param metadata Metadata to serialize
     * @param prettyFormat Whether to format output for human readability
     * @return Serialized metadata string or empty string on error
     */
    virtual std::string serializeMetadata(const StorageMetadata& metadata,
                                          bool prettyFormat = true) const = 0;

    /**
     * @brief Deserialize only metadata from string format
     * @param data Serialized string data
     * @param metadata Output metadata structure
     * @return true on success, false on error
     */
    virtual bool deserializeMetadata(const std::string& data,
                                     StorageMetadata& metadata) const = 0;

    /**
     * @brief Validate if the given string data is valid for this format
     * @param data String data to validate
     * @return true if valid, false otherwise
     */
    virtual bool validateFormat(const std::string& data) const = 0;

    /**
     * @brief Get the serialization format supported by this serializer
     * @return SerializationFormat enum value
     */
    virtual SerializationFormat getSupportedFormat() const noexcept = 0;

    /**
     * @brief Get the name of the underlying library being used
     * @return Library name (e.g., "Qt", "nlohmann_json", "pugixml")
     */
    virtual std::string getLibraryName() const noexcept = 0;

    /**
     * @brief Get the version of the underlying library being used
     * @return Library version string
     */
    virtual std::string getLibraryVersion() const noexcept = 0;

    /**
     * @brief Get the last error message from serialization/deserialization
     * @return Error message string or empty if no error
     */
    virtual std::string getLastError() const = 0;

    /**
     * @brief Check if the serializer supports pretty formatting
     * @return true if pretty formatting is supported
     */
    virtual bool supportsPrettyFormat() const noexcept = 0;

  protected:
    /**
     * @brief Set the last error message
     * @param error Error message to set
     */
    virtual void setLastError(const std::string& error) const = 0;
};

} // namespace MouseRecorder::Core::Serialization
