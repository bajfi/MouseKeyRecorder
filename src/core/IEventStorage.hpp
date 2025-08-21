// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include "Event.hpp"
#include <vector>
#include <memory>
#include <string>

namespace MouseRecorder::Core
{

/**
 * @brief Supported file formats for event storage
 */
enum class StorageFormat
{
    Json,
    Xml,
    Binary
};

/**
 * @brief Storage metadata containing additional information about the recording
 */
struct StorageMetadata
{
    std::string version{"0.0.1"};
    std::string applicationName{"MouseRecorder"};
    std::string createdBy;
    std::string description;
    uint64_t creationTimestamp{0};
    uint64_t totalDurationMs{0};
    size_t totalEvents{0};
    std::string platform;
    std::string screenResolution;
};

/**
 * @brief Interface for event storage and retrieval
 *
 * This interface defines the contract for saving and loading events
 * in different file formats. Implementations should handle format-specific
 * serialization and deserialization.
 */
class IEventStorage
{
  public:
    virtual ~IEventStorage() = default;

    /**
     * @brief Save events to file
     * @param events Vector of events to save
     * @param filename Path to the output file
     * @param metadata Optional metadata to include
     * @return true if save was successful
     */
    virtual bool saveEvents(const std::vector<std::unique_ptr<Event>>& events,
                            const std::string& filename,
                            const StorageMetadata& metadata = {}) = 0;

    /**
     * @brief Load events from file
     * @param filename Path to the input file
     * @param events Output vector to store loaded events
     * @param metadata Output metadata if available
     * @return true if load was successful
     */
    virtual bool loadEvents(const std::string& filename,
                            std::vector<std::unique_ptr<Event>>& events,
                            StorageMetadata& metadata) = 0;

    /**
     * @brief Get supported file format
     * @return format supported by this implementation
     */
    virtual StorageFormat getSupportedFormat() const noexcept = 0;

    /**
     * @brief Get recommended file extension
     * @return file extension (including dot)
     */
    virtual std::string getFileExtension() const noexcept = 0;

    /**
     * @brief Get format description
     * @return human-readable format description
     */
    virtual std::string getFormatDescription() const noexcept = 0;

    /**
     * @brief Validate file format without full loading
     * @param filename Path to the file to validate
     * @return true if file format is valid
     */
    virtual bool validateFile(const std::string& filename) const = 0;

    /**
     * @brief Get file metadata without loading events
     * @param filename Path to the file
     * @param metadata Output metadata
     * @return true if metadata was successfully read
     */
    virtual bool getFileMetadata(const std::string& filename,
                                 StorageMetadata& metadata) const = 0;

    /**
     * @brief Get the last error message if any operation failed
     * @return error message or empty string if no error
     */
    virtual std::string getLastError() const = 0;

    /**
     * @brief Set compression level (if supported by format)
     * @param level Compression level (0-9, format-dependent)
     */
    virtual void setCompressionLevel(int level) = 0;

    /**
     * @brief Check if format supports compression
     * @return true if compression is supported
     */
    virtual bool supportsCompression() const noexcept = 0;
};

} // namespace MouseRecorder::Core
