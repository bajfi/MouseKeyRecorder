// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "EventSerializerFactory.hpp"
#include "QtJsonEventSerializer.hpp"
#include "QtXmlEventSerializer.hpp"
#include "core/SpdlogConfig.hpp"

// Include third-party serializers based on CMake configuration
#ifdef MOUSERECORDER_USE_NLOHMANN_JSON
#include "NlohmannJsonEventSerializer.hpp"
#endif

#ifdef MOUSERECORDER_USE_PUGIXML
#include "PugixmlEventSerializer.hpp"
#endif

namespace MouseRecorder::Core::Serialization
{

std::unique_ptr<IEventSerializer> EventSerializerFactory::createSerializer(
    SerializationFormat format)
{
    return createSerializer(format, getDefaultLibraryPreference());
}

std::unique_ptr<IEventSerializer> EventSerializerFactory::createSerializer(
    SerializationFormat format, SerializationLibrary library)
{
    spdlog::debug("EventSerializerFactory: Creating serializer for format {} "
                  "with library {}",
                  static_cast<int>(format),
                  static_cast<int>(library));

    switch (format)
    {
    case SerializationFormat::Json:
        if (library == SerializationLibrary::Qt)
        {
            return createQtJsonSerializer();
        }
        else if (library == SerializationLibrary::ThirdParty)
        {
            return createNlohmannJsonSerializer();
        }
        break;

    case SerializationFormat::Xml:
        if (library == SerializationLibrary::Qt)
        {
            return createQtXmlSerializer();
        }
        else if (library == SerializationLibrary::ThirdParty)
        {
            return createPugixmlSerializer();
        }
        break;
    }

    spdlog::error("EventSerializerFactory: Unsupported serialization format {} "
                  "with library {}",
                  static_cast<int>(format),
                  static_cast<int>(library));
    return nullptr;
}

SerializationLibrary EventSerializerFactory::
    getDefaultLibraryPreference() noexcept
{
#ifdef MOUSERECORDER_DEFAULT_QT_SERIALIZATION
    return SerializationLibrary::Qt;
#else
    return SerializationLibrary::ThirdParty;
#endif
}

bool EventSerializerFactory::isSerializerAvailable(
    SerializationFormat format, SerializationLibrary library) noexcept
{
    switch (format)
    {
    case SerializationFormat::Json:
        if (library == SerializationLibrary::Qt)
        {
            return true; // Qt JSON is always available
        }
        else if (library == SerializationLibrary::ThirdParty)
        {
#ifdef MOUSERECORDER_USE_NLOHMANN_JSON
            return true;
#else
            return false;
#endif
        }
        break;

    case SerializationFormat::Xml:
        if (library == SerializationLibrary::Qt)
        {
            return true; // Qt XML is always available
        }
        else if (library == SerializationLibrary::ThirdParty)
        {
#ifdef MOUSERECORDER_USE_PUGIXML
            return true;
#else
            return false;
#endif
        }
        break;
    }

    return false;
}

std::vector<SerializationFormat> EventSerializerFactory::
    getAvailableFormats() noexcept
{
    std::vector<SerializationFormat> formats;

    // Check JSON availability
    if (isSerializerAvailable(SerializationFormat::Json,
                              SerializationLibrary::Qt) ||
        isSerializerAvailable(SerializationFormat::Json,
                              SerializationLibrary::ThirdParty))
    {
        formats.push_back(SerializationFormat::Json);
    }

    // Check XML availability
    if (isSerializerAvailable(SerializationFormat::Xml,
                              SerializationLibrary::Qt) ||
        isSerializerAvailable(SerializationFormat::Xml,
                              SerializationLibrary::ThirdParty))
    {
        formats.push_back(SerializationFormat::Xml);
    }

    return formats;
}

std::vector<SerializationLibrary> EventSerializerFactory::getAvailableLibraries(
    SerializationFormat format) noexcept
{
    std::vector<SerializationLibrary> libraries;

    if (isSerializerAvailable(format, SerializationLibrary::Qt))
    {
        libraries.push_back(SerializationLibrary::Qt);
    }

    if (isSerializerAvailable(format, SerializationLibrary::ThirdParty))
    {
        libraries.push_back(SerializationLibrary::ThirdParty);
    }

    return libraries;
}

std::string EventSerializerFactory::getFormatName(
    SerializationFormat format) noexcept
{
    switch (format)
    {
    case SerializationFormat::Json:
        return "JSON";
    case SerializationFormat::Xml:
        return "XML";
    default:
        return "Unknown";
    }
}

std::string EventSerializerFactory::getLibraryName(
    SerializationLibrary library) noexcept
{
    switch (library)
    {
    case SerializationLibrary::Qt:
        return "Qt";
    case SerializationLibrary::ThirdParty:
        return "Third-party";
    default:
        return "Unknown";
    }
}

std::unique_ptr<IEventSerializer> EventSerializerFactory::
    createQtJsonSerializer()
{
    try
    {
        return std::make_unique<QtJsonEventSerializer>();
    }
    catch (const std::exception& e)
    {
        spdlog::error(
            "EventSerializerFactory: Failed to create Qt JSON serializer: {}",
            e.what());
        return nullptr;
    }
}

std::unique_ptr<IEventSerializer> EventSerializerFactory::
    createQtXmlSerializer()
{
    try
    {
        return std::make_unique<QtXmlEventSerializer>();
    }
    catch (const std::exception& e)
    {
        spdlog::error(
            "EventSerializerFactory: Failed to create Qt XML serializer: {}",
            e.what());
        return nullptr;
    }
}

std::unique_ptr<IEventSerializer> EventSerializerFactory::
    createNlohmannJsonSerializer()
{
#ifdef MOUSERECORDER_USE_NLOHMANN_JSON
    try
    {
        return std::make_unique<NlohmannJsonEventSerializer>();
    }
    catch (const std::exception& e)
    {
        spdlog::error("EventSerializerFactory: Failed to create nlohmann JSON "
                      "serializer: {}",
                      e.what());
        return nullptr;
    }
#else
    spdlog::warn(
        "EventSerializerFactory: nlohmann::json serializer not available (not "
        "compiled with MOUSERECORDER_USE_NLOHMANN_JSON)");
    return nullptr;
#endif
}

std::unique_ptr<IEventSerializer> EventSerializerFactory::
    createPugixmlSerializer()
{
#ifdef MOUSERECORDER_USE_PUGIXML
    try
    {
        return std::make_unique<PugixmlEventSerializer>();
    }
    catch (const std::exception& e)
    {
        spdlog::error(
            "EventSerializerFactory: Failed to create pugixml serializer: {}",
            e.what());
        return nullptr;
    }
#else
    spdlog::warn("EventSerializerFactory: pugixml serializer not available "
                 "(not compiled with MOUSERECORDER_USE_PUGIXML)");
    return nullptr;
#endif
}

} // namespace MouseRecorder::Core::Serialization
