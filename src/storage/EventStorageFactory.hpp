// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include "core/IEventStorage.hpp"
#include <memory>
#include <optional>

namespace MouseRecorder::Storage
{

/**
 * @brief Factory class for creating event storage implementations
 *
 * This factory provides a centralized way to create storage handlers
 * for different file formats, following the Factory Method pattern.
 */
class EventStorageFactory
{
  public:
    /**
     * @brief Create storage handler for specific format
     * @param format Storage format
     * @return unique_ptr to storage handler or nullptr if format not supported
     */
    static std::unique_ptr<Core::IEventStorage> createStorage(
        Core::StorageFormat format);

    /**
     * @brief Create storage handler based on file extension
     * @param filename Filename with extension
     * @return unique_ptr to storage handler or nullptr if extension not
     * recognized
     */
    static std::unique_ptr<Core::IEventStorage> createStorageFromFilename(
        const std::string& filename);

    /**
     * @brief Get all supported storage formats
     * @return vector of supported formats
     */
    static std::vector<Core::StorageFormat> getSupportedFormats();

    /**
     * @brief Get file extension for format
     * @param format Storage format
     * @return file extension or empty string if format not supported
     */
    static std::string getFileExtension(Core::StorageFormat format);

    /**
     * @brief Get format description
     * @param format Storage format
     * @return human-readable description or empty string if format not
     * supported
     */
    static std::string getFormatDescription(Core::StorageFormat format);

    /**
     * @brief Check if format is supported
     * @param format Storage format to check
     * @return true if format is supported
     */
    static bool isFormatSupported(Core::StorageFormat format);

    /**
     * @brief Get format from file extension
     * @param extension File extension (with or without dot)
     * @return Storage format or std::nullopt if not recognized
     */
    static std::optional<Core::StorageFormat> getFormatFromExtension(
        const std::string& extension);

    /**
     * @brief Get file filter string for Qt file dialogs
     * @return filter string in Qt format
     */
    static std::string getFileDialogFilter();

  private:
    EventStorageFactory() = delete; // Static class
};

} // namespace MouseRecorder::Storage
