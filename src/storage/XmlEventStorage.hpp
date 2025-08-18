#pragma once

#include "core/IEventStorage.hpp"
#include <pugixml.hpp>

namespace MouseRecorder::Storage
{

/**
 * @brief XML implementation of event storage
 *
 * This class handles saving and loading events in XML format using pugixml.
 * The XML structure includes metadata and events in a human-readable format.
 */
class XmlEventStorage : public Core::IEventStorage
{
  public:
    XmlEventStorage();
    ~XmlEventStorage() override = default;

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
     * @brief Convert Event to XML node
     * @param event Event to convert
     * @param parent Parent XML node
     */
    void eventToXml(const Core::Event& event, pugi::xml_node& parent) const;

    /**
     * @brief Convert XML node to Event
     * @param node XML node
     * @return unique_ptr to Event or nullptr if conversion failed
     */
    std::unique_ptr<Core::Event> xmlToEvent(const pugi::xml_node& node) const;

    /**
     * @brief Convert StorageMetadata to XML node
     * @param metadata Metadata to convert
     * @param parent Parent XML node
     */
    void metadataToXml(const Core::StorageMetadata& metadata,
                       pugi::xml_node& parent) const;

    /**
     * @brief Convert XML node to StorageMetadata
     * @param node XML node
     * @return StorageMetadata
     */
    Core::StorageMetadata xmlToMetadata(const pugi::xml_node& node) const;

    /**
     * @brief Set last error message
     * @param error Error message
     */
    void setLastError(const std::string& error);

  private:
    mutable std::string m_lastError;
    bool m_prettyPrint{true};
};

} // namespace MouseRecorder::Storage
