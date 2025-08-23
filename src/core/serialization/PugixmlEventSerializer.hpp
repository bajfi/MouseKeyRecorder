// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include "IEventSerializer.hpp"
#include <pugixml.hpp>

namespace MouseRecorder::Core::Serialization
{

/**
 * @brief Pugixml-based XML implementation of event serialization
 *
 * This class wraps the existing pugixml-based serialization
 * logic from XmlEventStorage into the common IEventSerializer interface.
 * It provides a way to use the established third-party XML library
 * through the abstracted interface.
 */
class PugixmlEventSerializer : public IEventSerializer
{
  public:
    PugixmlEventSerializer();
    ~PugixmlEventSerializer() override = default;

    // IEventSerializer interface
    std::string serializeEvents(
        const std::vector<std::unique_ptr<Event>>& events,
        const StorageMetadata& metadata,
        bool prettyFormat = true) const override;

    bool deserializeEvents(const std::string& data,
                           std::vector<std::unique_ptr<Event>>& events,
                           StorageMetadata& metadata) const override;

    std::string serializeMetadata(const StorageMetadata& metadata,
                                  bool prettyFormat = true) const override;

    bool deserializeMetadata(const std::string& data,
                             StorageMetadata& metadata) const override;

    bool validateFormat(const std::string& data) const override;

    SerializationFormat getSupportedFormat() const noexcept override;
    std::string getLibraryName() const noexcept override;
    std::string getLibraryVersion() const noexcept override;
    std::string getLastError() const override;
    bool supportsPrettyFormat() const noexcept override;

  protected:
    void setLastError(const std::string& error) const override;

  private:
    /**
     * @brief Convert Event to XML node
     * @param event Event to convert
     * @param parent Parent XML node
     */
    void eventToXml(const Event& event, pugi::xml_node& parent) const;

    /**
     * @brief Convert XML node to Event
     * @param node XML node to convert
     * @return unique_ptr to Event or nullptr if conversion failed
     */
    std::unique_ptr<Event> xmlToEvent(const pugi::xml_node& node) const;

    /**
     * @brief Convert StorageMetadata to XML node
     * @param metadata Metadata to convert
     * @param parent Parent XML node
     */
    void metadataToXml(const StorageMetadata& metadata,
                       pugi::xml_node& parent) const;

    /**
     * @brief Convert XML node to StorageMetadata
     * @param node XML node to convert
     * @return StorageMetadata
     */
    StorageMetadata xmlToMetadata(const pugi::xml_node& node) const;

    /**
     * @brief Convert mouse button enum to string
     * @param button Mouse button enum
     * @return String representation
     */
    const char* mouseButtonToString(MouseButton button) const;

    /**
     * @brief Convert string to mouse button enum
     * @param buttonStr String representation
     * @return Mouse button enum
     */
    MouseButton stringToMouseButton(const char* buttonStr) const;

    mutable std::string m_lastError;
    bool m_prettyPrint{true};
};

} // namespace MouseRecorder::Core::Serialization
