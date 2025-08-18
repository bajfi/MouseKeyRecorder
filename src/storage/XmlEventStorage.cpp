#include "XmlEventStorage.hpp"
#include "core/Event.hpp"
#include <spdlog/spdlog.h>

namespace MouseRecorder::Storage
{

XmlEventStorage::XmlEventStorage()
{
    spdlog::debug("XmlEventStorage: Constructor");
}

bool XmlEventStorage::saveEvents(
    const std::vector<std::unique_ptr<Core::Event>>& events,
    const std::string& filename,
    const Core::StorageMetadata& metadata)
{
    spdlog::info(
        "XmlEventStorage: Saving {} events to {}", events.size(), filename);

    try
    {
        pugi::xml_document doc;

        // Create root element
        auto root = doc.append_child("MouseRecorderEvents");
        root.append_attribute("version") = "1.0";

        // Add metadata
        auto metadataNode = root.append_child("Metadata");
        metadataToXml(metadata, metadataNode);

        // Add events
        auto eventsNode = root.append_child("Events");
        eventsNode.append_attribute("count") =
            static_cast<unsigned int>(events.size());

        for (const auto& event : events)
        {
            if (event)
            {
                auto eventNode = eventsNode.append_child("Event");
                eventToXml(*event, eventNode);
            }
        }

        // Save to file
        if (auto result =
                doc.save_file(filename.c_str(),
                              m_prettyPrint ? "  " : "",
                              pugi::format_default | pugi::format_write_bom,
                              pugi::encoding_utf8);
            !result)
        {
            setLastError("Failed to save XML file: " + filename);
            return false;
        }

        spdlog::info("XmlEventStorage: Successfully saved {} events",
                     events.size());
        return true;
    }
    catch (const std::exception& e)
    {
        setLastError("XML serialization error: " + std::string(e.what()));
        return false;
    }
}

bool XmlEventStorage::loadEvents(
    const std::string& filename,
    std::vector<std::unique_ptr<Core::Event>>& events,
    Core::StorageMetadata& metadata)
{
    spdlog::info("XmlEventStorage: Loading events from {}", filename);

    try
    {
        pugi::xml_document doc;

        if (auto result = doc.load_file(filename.c_str()); !result)
        {
            setLastError("Failed to parse XML file: " +
                         std::string(result.description()));
            return false;
        }

        auto root = doc.child("MouseRecorderEvents");
        if (!root)
        {
            setLastError("Invalid XML format: missing root element");
            return false;
        }

        // Load metadata
        if (auto metadataNode = root.child("Metadata"); metadataNode)
        {
            metadata = xmlToMetadata(metadataNode);
        }

        // Load events
        events.clear();
        if (auto eventsNode = root.child("Events"); eventsNode)
        {
            for (auto eventNode : eventsNode.children("Event"))
            {
                auto event = xmlToEvent(eventNode);
                if (event)
                {
                    events.push_back(std::move(event));
                }
            }
        }

        spdlog::info("XmlEventStorage: Successfully loaded {} events",
                     events.size());
        return true;
    }
    catch (const std::exception& e)
    {
        setLastError("XML deserialization error: " + std::string(e.what()));
        return false;
    }
}

Core::StorageFormat XmlEventStorage::getSupportedFormat() const noexcept
{
    return Core::StorageFormat::Xml;
}

std::string XmlEventStorage::getFileExtension() const noexcept
{
    return ".xml";
}

std::string XmlEventStorage::getFormatDescription() const noexcept
{
    return "XML Event Recording";
}

bool XmlEventStorage::validateFile(const std::string& filename) const
{
    try
    {
        pugi::xml_document doc;

        if (auto result = doc.load_file(filename.c_str()); !result)
        {
            return false;
        }

        auto root = doc.child("MouseRecorderEvents");
        return root && root.child("Events");
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool XmlEventStorage::getFileMetadata(const std::string& filename,
                                      Core::StorageMetadata& metadata) const
{
    try
    {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_file(filename.c_str());

        if (!result)
        {
            return false;
        }

        auto root = doc.child("MouseRecorderEvents");
        if (!root)
        {
            return false;
        }

        if (auto metadataNode = root.child("Metadata"); metadataNode)
        {
            metadata = xmlToMetadata(metadataNode);
            return true;
        }

        return false;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

std::string XmlEventStorage::getLastError() const
{
    return m_lastError;
}

void XmlEventStorage::setCompressionLevel(int level)
{
    // XML doesn't support compression directly, but we can control formatting
    m_prettyPrint = (level > 0);
}

bool XmlEventStorage::supportsCompression() const noexcept
{
    return false; // XML itself doesn't support compression
}

void XmlEventStorage::eventToXml(const Core::Event& event,
                                 pugi::xml_node& parent) const
{
    parent.append_attribute("timestamp") = event.getTimestampMs();

    switch (event.getType())
    {
    case Core::EventType::MouseMove: {
        parent.append_attribute("type") = "mouse_move";
        const auto* data = event.getMouseData();
        auto pos = parent.append_child("Position");
        pos.append_attribute("x") = data->position.x;
        pos.append_attribute("y") = data->position.y;
        if (static_cast<uint32_t>(data->modifiers) != 0)
        {
            parent.append_attribute("modifiers") =
                static_cast<uint32_t>(data->modifiers);
        }
        break;
    }

    case Core::EventType::MouseClick: {
        parent.append_attribute("type") = "mouse_click";
        const auto* data = event.getMouseData();
        auto pos = parent.append_child("Position");
        pos.append_attribute("x") = data->position.x;
        pos.append_attribute("y") = data->position.y;

        const char* buttonName = "left";
        switch (data->button)
        {
        case Core::MouseButton::Left:
            buttonName = "left";
            break;
        case Core::MouseButton::Right:
            buttonName = "right";
            break;
        case Core::MouseButton::Middle:
            buttonName = "middle";
            break;
        case Core::MouseButton::X1:
            buttonName = "x1";
            break;
        case Core::MouseButton::X2:
            buttonName = "x2";
            break;
        }
        parent.append_attribute("button") = buttonName;

        if (static_cast<uint32_t>(data->modifiers) != 0)
        {
            parent.append_attribute("modifiers") =
                static_cast<uint32_t>(data->modifiers);
        }
        break;
    }

    case Core::EventType::MouseDoubleClick: {
        parent.append_attribute("type") = "mouse_double_click";
        const auto* data = event.getMouseData();
        auto pos = parent.append_child("Position");
        pos.append_attribute("x") = data->position.x;
        pos.append_attribute("y") = data->position.y;

        const char* buttonName = "left";
        switch (data->button)
        {
        case Core::MouseButton::Left:
            buttonName = "left";
            break;
        case Core::MouseButton::Right:
            buttonName = "right";
            break;
        case Core::MouseButton::Middle:
            buttonName = "middle";
            break;
        case Core::MouseButton::X1:
            buttonName = "x1";
            break;
        case Core::MouseButton::X2:
            buttonName = "x2";
            break;
        }
        parent.append_attribute("button") = buttonName;

        if (static_cast<uint32_t>(data->modifiers) != 0)
        {
            parent.append_attribute("modifiers") =
                static_cast<uint32_t>(data->modifiers);
        }
        break;
    }

    case Core::EventType::MouseWheel: {
        parent.append_attribute("type") = "mouse_wheel";
        const auto* data = event.getMouseData();
        auto pos = parent.append_child("Position");
        pos.append_attribute("x") = data->position.x;
        pos.append_attribute("y") = data->position.y;
        parent.append_attribute("wheel_delta") = data->wheelDelta;

        if (static_cast<uint32_t>(data->modifiers) != 0)
        {
            parent.append_attribute("modifiers") =
                static_cast<uint32_t>(data->modifiers);
        }
        break;
    }

    case Core::EventType::KeyPress: {
        parent.append_attribute("type") = "key_press";
        const auto* data = event.getKeyboardData();
        parent.append_attribute("key_code") = data->keyCode;
        parent.append_attribute("key_name") = data->keyName.c_str();

        if (static_cast<uint32_t>(data->modifiers) != 0)
        {
            parent.append_attribute("modifiers") =
                static_cast<uint32_t>(data->modifiers);
        }
        if (data->isRepeated)
        {
            parent.append_attribute("repeated") = true;
        }
        break;
    }

    case Core::EventType::KeyRelease: {
        parent.append_attribute("type") = "key_release";
        const auto* data = event.getKeyboardData();
        parent.append_attribute("key_code") = data->keyCode;
        parent.append_attribute("key_name") = data->keyName.c_str();

        if (static_cast<uint32_t>(data->modifiers) != 0)
        {
            parent.append_attribute("modifiers") =
                static_cast<uint32_t>(data->modifiers);
        }
        break;
    }

    case Core::EventType::KeyCombination: {
        parent.append_attribute("type") = "key_combination";
        const auto* data = event.getKeyboardData();
        parent.append_attribute("key_code") = data->keyCode;
        parent.append_attribute("key_name") = data->keyName.c_str();

        if (static_cast<uint32_t>(data->modifiers) != 0)
        {
            parent.append_attribute("modifiers") =
                static_cast<uint32_t>(data->modifiers);
        }
        break;
    }
    }
}

std::unique_ptr<Core::Event> XmlEventStorage::xmlToEvent(
    const pugi::xml_node& node) const
{
    try
    {
        std::string typeStr = node.attribute("type").value();
        uint64_t timestamp = node.attribute("timestamp").as_ullong();
        auto timePoint = Core::Event::timestampFromMs(timestamp);

        if (typeStr == "mouse_move")
        {
            Core::MouseEventData data;
            auto pos = node.child("Position");
            data.position.x = pos.attribute("x").as_int();
            data.position.y = pos.attribute("y").as_int();
            data.modifiers = static_cast<Core::KeyModifier>(
                node.attribute("modifiers").as_uint());

            return std::make_unique<Core::Event>(
                Core::EventType::MouseMove, data, timePoint);
        }
        if (typeStr == "mouse_click")
        {
            Core::MouseEventData data;
            auto pos = node.child("Position");
            data.position.x = pos.attribute("x").as_int();
            data.position.y = pos.attribute("y").as_int();

            if (std::string_view buttonStr = node.attribute("button").value();
                buttonStr == "left")
                data.button = Core::MouseButton::Left;
            else if (buttonStr == "right")
                data.button = Core::MouseButton::Right;
            else if (buttonStr == "middle")
                data.button = Core::MouseButton::Middle;
            else if (buttonStr == "x1")
                data.button = Core::MouseButton::X1;
            else if (buttonStr == "x2")
                data.button = Core::MouseButton::X2;

            data.modifiers = static_cast<Core::KeyModifier>(
                node.attribute("modifiers").as_uint());

            return std::make_unique<Core::Event>(
                Core::EventType::MouseClick, data, timePoint);
        }
        if (typeStr == "mouse_double_click")
        {
            Core::MouseEventData data;
            auto pos = node.child("Position");
            data.position.x = pos.attribute("x").as_int();
            data.position.y = pos.attribute("y").as_int();

            if (std::string_view buttonStr = node.attribute("button").value();
                buttonStr == "left")
                data.button = Core::MouseButton::Left;
            else if (buttonStr == "right")
                data.button = Core::MouseButton::Right;
            else if (buttonStr == "middle")
                data.button = Core::MouseButton::Middle;
            else if (buttonStr == "x1")
                data.button = Core::MouseButton::X1;
            else if (buttonStr == "x2")
                data.button = Core::MouseButton::X2;

            data.modifiers = static_cast<Core::KeyModifier>(
                node.attribute("modifiers").as_uint());

            return std::make_unique<Core::Event>(
                Core::EventType::MouseDoubleClick, data, timePoint);
        }
        if (typeStr == "mouse_wheel")
        {
            Core::MouseEventData data;
            auto pos = node.child("Position");
            data.position.x = pos.attribute("x").as_int();
            data.position.y = pos.attribute("y").as_int();
            data.wheelDelta = node.attribute("wheel_delta").as_int();
            data.modifiers = static_cast<Core::KeyModifier>(
                node.attribute("modifiers").as_uint());

            return std::make_unique<Core::Event>(
                Core::EventType::MouseWheel, data, timePoint);
        }
        if (typeStr == "key_press")
        {
            Core::KeyboardEventData data;
            data.keyCode = node.attribute("key_code").as_uint();
            data.keyName = node.attribute("key_name").value();
            data.modifiers = static_cast<Core::KeyModifier>(
                node.attribute("modifiers").as_uint());
            data.isRepeated = node.attribute("repeated").as_bool();

            return std::make_unique<Core::Event>(
                Core::EventType::KeyPress, data, timePoint);
        }
        if (typeStr == "key_release")
        {
            Core::KeyboardEventData data;
            data.keyCode = node.attribute("key_code").as_uint();
            data.keyName = node.attribute("key_name").value();
            data.modifiers = static_cast<Core::KeyModifier>(
                node.attribute("modifiers").as_uint());

            return std::make_unique<Core::Event>(
                Core::EventType::KeyRelease, data, timePoint);
        }
        if (typeStr == "key_combination")
        {
            Core::KeyboardEventData data;
            data.keyCode = node.attribute("key_code").as_uint();
            data.keyName = node.attribute("key_name").value();
            data.modifiers = static_cast<Core::KeyModifier>(
                node.attribute("modifiers").as_uint());

            return std::make_unique<Core::Event>(
                Core::EventType::KeyCombination, data, timePoint);
        }

        return nullptr;
    }
    catch (const std::exception&)
    {
        return nullptr;
    }
}

void XmlEventStorage::metadataToXml(const Core::StorageMetadata& metadata,
                                    pugi::xml_node& parent) const
{
    parent.append_child("Version").text() = metadata.version.c_str();
    parent.append_child("ApplicationName").text() =
        metadata.applicationName.c_str();
    parent.append_child("CreatedBy").text() = metadata.createdBy.c_str();
    parent.append_child("Description").text() = metadata.description.c_str();
    parent.append_child("CreationTimestamp").text() =
        metadata.creationTimestamp;
    parent.append_child("TotalDurationMs").text() = metadata.totalDurationMs;
    parent.append_child("TotalEvents").text() = metadata.totalEvents;
    parent.append_child("Platform").text() = metadata.platform.c_str();
    parent.append_child("ScreenResolution").text() =
        metadata.screenResolution.c_str();
}

Core::StorageMetadata XmlEventStorage::xmlToMetadata(
    const pugi::xml_node& node) const
{
    Core::StorageMetadata metadata;

    metadata.version = node.child("Version").text().as_string();
    metadata.applicationName = node.child("ApplicationName").text().as_string();
    metadata.createdBy = node.child("CreatedBy").text().as_string();
    metadata.description = node.child("Description").text().as_string();
    metadata.creationTimestamp =
        node.child("CreationTimestamp").text().as_ullong();
    metadata.totalDurationMs = node.child("TotalDurationMs").text().as_ullong();
    metadata.totalEvents = node.child("TotalEvents").text().as_ullong();
    metadata.platform = node.child("Platform").text().as_string();
    metadata.screenResolution =
        node.child("ScreenResolution").text().as_string();

    return metadata;
}

void XmlEventStorage::setLastError(const std::string& error)
{
    m_lastError = error;
    spdlog::error("XmlEventStorage: {}", error);
}

} // namespace MouseRecorder::Storage
