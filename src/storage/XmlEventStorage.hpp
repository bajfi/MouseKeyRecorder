// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include "core/IEventStorage.hpp"
#include "core/serialization/IEventSerializer.hpp"
#include <memory>

namespace MouseRecorder::Storage
{

/**
 * @brief XML implementation of event storage
 *
 * This class handles saving and loading events in XML format using
 * the abstract serialization framework. It can work with Qt's XML
 * serialization or third-party libraries like pugixml.
 */
class XmlEventStorage : public Core::IEventStorage
{
  public:
    /**
     * @brief Default constructor using factory-created XML serializer
     */
    XmlEventStorage();

    /**
     * @brief Constructor with custom XML serializer
     * @param serializer Custom XML serializer implementation
     */
    explicit XmlEventStorage(
        std::unique_ptr<Core::Serialization::IEventSerializer> serializer);

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
     * @brief Set last error message
     * @param error Error message
     */
    void setLastError(const std::string& error);

  private:
    mutable std::string m_lastError;
    std::unique_ptr<Core::Serialization::IEventSerializer> m_serializer;
};

} // namespace MouseRecorder::Storage
