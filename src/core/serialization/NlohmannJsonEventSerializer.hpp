// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include "IEventSerializer.hpp"
#include <nlohmann/json.hpp>

namespace MouseRecorder::Core::Serialization
{

/**
 * @brief Nlohmann JSON implementation of event serialization
 *
 * This class wraps the existing nlohmann::json-based serialization
 * logic from JsonEventStorage into the common IEventSerializer interface.
 * It provides a way to use the established third-party JSON library
 * through the abstracted interface.
 */
class NlohmannJsonEventSerializer : public IEventSerializer
{
  public:
    NlohmannJsonEventSerializer();
    ~NlohmannJsonEventSerializer() override = default;

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
    using json = nlohmann::json;

    /**
     * @brief Convert Event to JSON object
     * @param event Event to convert
     * @return JSON representation
     */
    json eventToJson(const Event& event) const;

    /**
     * @brief Convert JSON object to Event
     * @param json JSON object
     * @return unique_ptr to Event or nullptr if conversion failed
     */
    std::unique_ptr<Event> jsonToEvent(const json& json) const;

    /**
     * @brief Convert StorageMetadata to JSON object
     * @param metadata Metadata to convert
     * @return JSON representation
     */
    json metadataToJson(const StorageMetadata& metadata) const;

    /**
     * @brief Convert JSON object to StorageMetadata
     * @param json JSON object to convert
     * @return StorageMetadata
     */
    StorageMetadata jsonToMetadata(const json& json) const;

    /**
     * @brief Convert MouseEventData to JSON
     * @param data Mouse event data
     * @return JSON representation
     */
    json mouseEventDataToJson(const MouseEventData& data) const;

    /**
     * @brief Convert JSON to MouseEventData
     * @param json JSON object
     * @return MouseEventData
     */
    MouseEventData jsonToMouseEventData(const json& json) const;

    /**
     * @brief Convert KeyboardEventData to JSON
     * @param data Keyboard event data
     * @return JSON representation
     */
    json keyboardEventDataToJson(const KeyboardEventData& data) const;

    /**
     * @brief Convert JSON to KeyboardEventData
     * @param json JSON object
     * @return KeyboardEventData
     */
    KeyboardEventData jsonToKeyboardEventData(const json& json) const;

    mutable std::string m_lastError;
    int m_indentLevel{2};
};

} // namespace MouseRecorder::Core::Serialization
