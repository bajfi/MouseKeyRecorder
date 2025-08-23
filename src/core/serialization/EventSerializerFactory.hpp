// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include "IEventSerializer.hpp"
#include <memory>
#include <string>

namespace MouseRecorder::Core::Serialization
{

/**
 * @brief Serialization implementation preference
 */
enum class SerializationLibrary
{
    Qt,
    ThirdParty
};

/**
 * @brief Factory for creating event serializers
 *
 * This factory creates appropriate serializers based on the requested format
 * and library preference. It allows choosing between Qt-based and third-party
 * library implementations at runtime or compile time based on CMake options.
 */
class EventSerializerFactory
{
  public:
    /**
     * @brief Create serializer for specific format using default library
     * preference
     * @param format Serialization format (JSON or XML)
     * @return unique_ptr to serializer or nullptr if not available
     */
    static std::unique_ptr<IEventSerializer> createSerializer(
        SerializationFormat format);

    /**
     * @brief Create serializer for specific format and library preference
     * @param format Serialization format (JSON or XML)
     * @param library Library preference (Qt or ThirdParty)
     * @return unique_ptr to serializer or nullptr if not available
     */
    static std::unique_ptr<IEventSerializer> createSerializer(
        SerializationFormat format, SerializationLibrary library);

    /**
     * @brief Get default library preference based on CMake configuration
     * @return Default serialization library
     */
    static SerializationLibrary getDefaultLibraryPreference() noexcept;

    /**
     * @brief Check if a specific serializer is available
     * @param format Serialization format
     * @param library Library preference
     * @return true if available, false otherwise
     */
    static bool isSerializerAvailable(SerializationFormat format,
                                      SerializationLibrary library) noexcept;

    /**
     * @brief Get list of available serialization formats
     * @return Vector of available formats
     */
    static std::vector<SerializationFormat> getAvailableFormats() noexcept;

    /**
     * @brief Get list of available libraries for a specific format
     * @param format Serialization format
     * @return Vector of available libraries for the format
     */
    static std::vector<SerializationLibrary> getAvailableLibraries(
        SerializationFormat format) noexcept;

    /**
     * @brief Get human-readable name for serialization format
     * @param format Serialization format
     * @return Format name string
     */
    static std::string getFormatName(SerializationFormat format) noexcept;

    /**
     * @brief Get human-readable name for serialization library
     * @param library Serialization library
     * @return Library name string
     */
    static std::string getLibraryName(SerializationLibrary library) noexcept;

  private:
    EventSerializerFactory() = default;

    /**
     * @brief Create Qt-based JSON serializer
     * @return unique_ptr to serializer or nullptr if not available
     */
    static std::unique_ptr<IEventSerializer> createQtJsonSerializer();

    /**
     * @brief Create Qt-based XML serializer
     * @return unique_ptr to serializer or nullptr if not available
     */
    static std::unique_ptr<IEventSerializer> createQtXmlSerializer();

    /**
     * @brief Create nlohmann::json-based JSON serializer
     * @return unique_ptr to serializer or nullptr if not available
     */
    static std::unique_ptr<IEventSerializer> createNlohmannJsonSerializer();

    /**
     * @brief Create pugixml-based XML serializer
     * @return unique_ptr to serializer or nullptr if not available
     */
    static std::unique_ptr<IEventSerializer> createPugixmlSerializer();
};

} // namespace MouseRecorder::Core::Serialization
