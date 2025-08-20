// Copyright (c) 2025 JackLee
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "EventStorageFactory.hpp"
#include "JsonEventStorage.hpp"
#include "BinaryEventStorage.hpp"
#include "XmlEventStorage.hpp"
#include "core/SpdlogConfig.hpp"
#include <algorithm>
#include <filesystem>

namespace MouseRecorder::Storage
{

std::unique_ptr<Core::IEventStorage> EventStorageFactory::createStorage(
    Core::StorageFormat format)
{
    spdlog::debug("EventStorageFactory: Creating storage for format {}",
                  static_cast<int>(format));

    switch (format)
    {
    case Core::StorageFormat::Json:
        return std::make_unique<JsonEventStorage>();

    case Core::StorageFormat::Binary:
        return std::make_unique<BinaryEventStorage>();

    case Core::StorageFormat::Xml:
        return std::make_unique<XmlEventStorage>();

    default:
        spdlog::error("EventStorageFactory: Unsupported storage format {}",
                      static_cast<int>(format));
        return nullptr;
    }
}

std::unique_ptr<Core::IEventStorage> EventStorageFactory::
    createStorageFromFilename(const std::string& filename)
{
    std::filesystem::path path(filename);
    std::string extension = path.extension().string();

    // Convert to lowercase for comparison
    std::transform(
        extension.begin(), extension.end(), extension.begin(), ::tolower);

    auto format = getFormatFromExtension(extension);
    if (format)
    {
        return createStorage(*format);
    }

    spdlog::warn(
        "EventStorageFactory: Unknown file extension '{}', defaulting to JSON",
        extension);
    return createStorage(Core::StorageFormat::Json);
}

std::vector<Core::StorageFormat> EventStorageFactory::getSupportedFormats()
{
    return {Core::StorageFormat::Json,
            Core::StorageFormat::Binary,
            Core::StorageFormat::Xml};
}

std::string EventStorageFactory::getFileExtension(Core::StorageFormat format)
{
    auto storage = createStorage(format);
    if (storage)
    {
        return storage->getFileExtension();
    }
    return "";
}

std::string EventStorageFactory::getFormatDescription(
    Core::StorageFormat format)
{
    auto storage = createStorage(format);
    if (storage)
    {
        return storage->getFormatDescription();
    }
    return "";
}

bool EventStorageFactory::isFormatSupported(Core::StorageFormat format)
{
    auto supportedFormats = getSupportedFormats();
    return std::find(supportedFormats.begin(),
                     supportedFormats.end(),
                     format) != supportedFormats.end();
}

std::optional<Core::StorageFormat> EventStorageFactory::getFormatFromExtension(
    const std::string& extension)
{
    std::string ext = extension;

    // Remove leading dot if present
    if (!ext.empty() && ext[0] == '.')
    {
        ext = ext.substr(1);
    }

    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "json")
    {
        return Core::StorageFormat::Json;
    }
    if (ext == "mre")
    {
        return Core::StorageFormat::Binary;
    }
    if (ext == "xml")
    {
        return Core::StorageFormat::Xml;
    }

    return std::nullopt;
}

std::string EventStorageFactory::getFileDialogFilter()
{
    std::string filter;

    auto formats = getSupportedFormats();
    for (size_t i = 0; i < formats.size(); ++i)
    {
        if (i > 0)
        {
            filter += ";;";
        }

        std::string description = getFormatDescription(formats[i]);
        std::string extension = getFileExtension(formats[i]);

        filter += description + " (*" + extension + ")";
    }

    // Add "All supported files" option
    if (!formats.empty())
    {
        std::string allExtensions;
        for (size_t i = 0; i < formats.size(); ++i)
        {
            if (i > 0)
            {
                allExtensions += " ";
            }
            allExtensions += "*" + getFileExtension(formats[i]);
        }

        filter = "All supported files (" + allExtensions + ");;" + filter;
    }

    return filter;
}

} // namespace MouseRecorder::Storage
