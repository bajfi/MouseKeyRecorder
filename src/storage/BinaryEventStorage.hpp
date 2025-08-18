#pragma once

#include "core/IEventStorage.hpp"
#include <cstdint>

namespace MouseRecorder::Storage
{

/**
 * @brief Binary implementation of event storage
 *
 * This class handles saving and loading events in a custom binary format
 * for optimal performance and minimal file size.
 *
 * Binary Format:
 * - Header: Magic number (4 bytes) + Version (4 bytes) + Metadata size (4
 * bytes)
 * - Metadata: Serialized metadata structure
 * - Event count (4 bytes)
 * - Events: Array of serialized events
 */
class BinaryEventStorage : public Core::IEventStorage
{
  public:
    BinaryEventStorage();
    ~BinaryEventStorage() override = default;

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
    static constexpr uint32_t MAGIC_NUMBER =
        0x4D525245; // "MRRE" - MouseRecorder Recording Events
    static constexpr uint32_t FORMAT_VERSION = 1;

    /**
     * @brief Serialize an event to binary buffer
     * @param event Event to serialize
     * @param buffer Output buffer
     */
    void serializeEvent(const Core::Event& event,
                        std::vector<uint8_t>& buffer) const;

    /**
     * @brief Deserialize an event from binary buffer
     * @param buffer Input buffer
     * @param offset Current offset in buffer (will be updated)
     * @return unique_ptr to Event or nullptr if deserialization failed
     */
    std::unique_ptr<Core::Event> deserializeEvent(
        const std::vector<uint8_t>& buffer, size_t& offset) const;

    /**
     * @brief Serialize metadata to binary buffer
     * @param metadata Metadata to serialize
     * @param buffer Output buffer
     */
    void serializeMetadata(const Core::StorageMetadata& metadata,
                           std::vector<uint8_t>& buffer) const;

    /**
     * @brief Deserialize metadata from binary buffer
     * @param buffer Input buffer
     * @param offset Current offset in buffer (will be updated)
     * @return StorageMetadata
     */
    Core::StorageMetadata deserializeMetadata(
        const std::vector<uint8_t>& buffer, size_t& offset) const;

    /**
     * @brief Write binary data to buffer (little-endian)
     */
    template <typename T>
    void writeBinary(std::vector<uint8_t>& buffer, const T& value) const;

    /**
     * @brief Read binary data from buffer (little-endian)
     */
    template <typename T>
    T readBinary(const std::vector<uint8_t>& buffer, size_t& offset) const;

    /**
     * @brief Write string to buffer (length-prefixed)
     */
    void writeString(std::vector<uint8_t>& buffer,
                     const std::string& str) const;

    /**
     * @brief Read string from buffer (length-prefixed)
     */
    std::string readString(const std::vector<uint8_t>& buffer,
                           size_t& offset) const;

    /**
     * @brief Compress data using simple RLE compression
     * @param input Input data
     * @return Compressed data
     */
    std::vector<uint8_t> compressData(const std::vector<uint8_t>& input) const;

    /**
     * @brief Decompress RLE compressed data
     * @param input Compressed data
     * @return Decompressed data
     */
    std::vector<uint8_t> decompressData(
        const std::vector<uint8_t>& input) const;

    /**
     * @brief Set last error message
     * @param error Error message
     */
    void setLastError(const std::string& error);

  private:
    mutable std::string m_lastError;
    bool m_compressionEnabled{false};
};

} // namespace MouseRecorder::Storage
