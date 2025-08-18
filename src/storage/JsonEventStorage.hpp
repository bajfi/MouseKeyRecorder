#pragma once

#include "core/IEventStorage.hpp"
#include <nlohmann/json.hpp>

namespace MouseRecorder::Storage
{

/**
 * @brief JSON implementation of event storage
 *
 * This class handles saving and loading events in JSON format.
 * The JSON structure includes metadata and an array of events.
 */
class JsonEventStorage : public Core::IEventStorage
{
  public:
    JsonEventStorage();
    ~JsonEventStorage() override = default;

    // IEventStorage interface
    bool saveEvents(const std::vector<std::unique_ptr<Core::Event>>& events,
                    const std::string& filename,
                    const Core::StorageMetadata& metadata = {}) override;

    bool loadEvents(const std::string& filename,
                    std::vector<std::unique_ptr<Core::Event>>& events,
                    Core::StorageMetadata& metadata) override;

    Core::StorageFormat getSupportedFormat() const noexcept override;
    std::string getFileExtension() const noexcept override;
    std::string getFormatDescription() const noexcept override;
    bool validateFile(const std::string& filename) const override;
    bool getFileMetadata(const std::string& filename,
                         Core::StorageMetadata& metadata) const override;
    std::string getLastError() const override;
    void setCompressionLevel(int level) override;
    bool supportsCompression() const noexcept override;

  private:
    /**
     * @brief Convert Event to JSON object
     * @param event Event to convert
     * @return JSON representation
     */
    nlohmann::json eventToJson(const Core::Event& event) const;

    /**
     * @brief Convert JSON object to Event
     * @param json JSON object
     * @return unique_ptr to Event or nullptr if conversion failed
     */
    std::unique_ptr<Core::Event> jsonToEvent(const nlohmann::json& json) const;

    /**
     * @brief Convert StorageMetadata to JSON object
     * @param metadata Metadata to convert
     * @return JSON representation
     */
    nlohmann::json metadataToJson(const Core::StorageMetadata& metadata) const;

    /**
     * @brief Convert JSON object to StorageMetadata
     * @param json JSON object
     * @return StorageMetadata
     */
    Core::StorageMetadata jsonToMetadata(const nlohmann::json& json) const;

    /**
     * @brief Convert MouseEventData to JSON
     * @param data Mouse event data
     * @return JSON representation
     */
    nlohmann::json mouseEventDataToJson(const Core::MouseEventData& data) const;

    /**
     * @brief Convert JSON to MouseEventData
     * @param json JSON object
     * @return MouseEventData
     */
    Core::MouseEventData jsonToMouseEventData(const nlohmann::json& json) const;

    /**
     * @brief Convert KeyboardEventData to JSON
     * @param data Keyboard event data
     * @return JSON representation
     */
    nlohmann::json keyboardEventDataToJson(
        const Core::KeyboardEventData& data) const;

    /**
     * @brief Convert JSON to KeyboardEventData
     * @param json JSON object
     * @return KeyboardEventData
     */
    Core::KeyboardEventData jsonToKeyboardEventData(
        const nlohmann::json& json) const;

    /**
     * @brief Set last error message
     * @param error Error message
     */
    void setLastError(const std::string& error);

  private:
    mutable std::string m_lastError;
    int m_indentLevel{2}; // JSON indentation level for pretty printing
};

} // namespace MouseRecorder::Storage
