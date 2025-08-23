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
 * @brief JSON implementation of event storage
 *
 * This class handles saving and loading events in JSON format using
 * the IEventSerializer interface for abstracted serialization.
 * The storage class now focuses on file I/O while delegating
 * serialization/deserialization to the serializer.
 */
class JsonEventStorage : public Core::IEventStorage
{
  public:
    JsonEventStorage();

    /**
     * @brief Constructor with custom serializer
     * @param serializer Custom JSON serializer implementation
     */
    explicit JsonEventStorage(
        std::unique_ptr<Core::Serialization::IEventSerializer> serializer);

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
     * @brief Set last error message
     * @param error Error message
     */
    void setLastError(const std::string& error);

  private:
    mutable std::string m_lastError;
    std::unique_ptr<Core::Serialization::IEventSerializer> m_serializer;
};

} // namespace MouseRecorder::Storage
