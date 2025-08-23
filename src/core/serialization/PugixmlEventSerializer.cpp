// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "PugixmlEventSerializer.hpp"
#include "core/Event.hpp"
#include "core/SpdlogConfig.hpp"
#include <sstream>

namespace MouseRecorder::Core::Serialization
{

PugixmlEventSerializer::PugixmlEventSerializer()
{
    spdlog::debug("PugixmlEventSerializer: Constructor");
}

std::string PugixmlEventSerializer::serializeEvents(
    const std::vector<std::unique_ptr<Event>>& events,
    const StorageMetadata& metadata,
    bool prettyFormat) const
{
    try
    {
        pugi::xml_document doc;

        // Create root element
        auto root = doc.append_child("MouseRecorderEvents");

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

        // Convert to string
        std::ostringstream oss;
        doc.save(oss,
                 prettyFormat ? "  " : "",
                 pugi::format_default | pugi::format_write_bom,
                 pugi::encoding_utf8);

        return oss.str();
    }
    catch (const std::exception& e)
    {
        setLastError(std::string("Failed to serialize events: ") + e.what());
        return "";
    }
}

bool PugixmlEventSerializer::deserializeEvents(
    const std::string& data,
    std::vector<std::unique_ptr<Event>>& events,
    StorageMetadata& metadata) const
{
    try
    {
        pugi::xml_document doc;

        if (auto result = doc.load_string(data.c_str()); !result)
        {
            setLastError("Failed to parse XML: " +
                         std::string(result.description()));
            return false;
        }

        auto root = doc.child("MouseRecorderEvents");
        if (!root)
        {
            setLastError("Invalid XML format: missing root element");
            return false;
        }

        // Parse metadata
        auto metadataNode = root.child("Metadata");
        if (metadataNode)
        {
            metadata = xmlToMetadata(metadataNode);
        }

        // Parse events
        events.clear();
        auto eventsNode = root.child("Events");
        if (eventsNode)
        {
            for (auto eventNode = eventsNode.child("Event"); eventNode;
                 eventNode = eventNode.next_sibling("Event"))
            {
                auto event = xmlToEvent(eventNode);
                if (event)
                {
                    events.push_back(std::move(event));
                }
            }
        }

        return true;
    }
    catch (const std::exception& e)
    {
        setLastError(std::string("Failed to deserialize events: ") + e.what());
        return false;
    }
}

std::string PugixmlEventSerializer::serializeMetadata(
    const StorageMetadata& metadata, bool prettyFormat) const
{
    try
    {
        pugi::xml_document doc;
        auto metadataNode = doc.append_child("Metadata");
        metadataToXml(metadata, metadataNode);

        std::ostringstream oss;
        doc.save(oss,
                 prettyFormat ? "  " : "",
                 pugi::format_default | pugi::format_write_bom,
                 pugi::encoding_utf8);

        return oss.str();
    }
    catch (const std::exception& e)
    {
        setLastError(std::string("Failed to serialize metadata: ") + e.what());
        return "";
    }
}

bool PugixmlEventSerializer::deserializeMetadata(
    const std::string& data, StorageMetadata& metadata) const
{
    try
    {
        pugi::xml_document doc;

        if (auto result = doc.load_string(data.c_str()); !result)
        {
            setLastError("Failed to parse XML: " +
                         std::string(result.description()));
            return false;
        }

        auto metadataNode = doc.child("Metadata");
        if (metadataNode)
        {
            metadata = xmlToMetadata(metadataNode);
            return true;
        }

        setLastError("No metadata found in XML");
        return false;
    }
    catch (const std::exception& e)
    {
        setLastError(std::string("Failed to deserialize metadata: ") +
                     e.what());
        return false;
    }
}

bool PugixmlEventSerializer::validateFormat(const std::string& data) const
{
    pugi::xml_document doc;
    return doc.load_string(data.c_str());
}

SerializationFormat PugixmlEventSerializer::getSupportedFormat() const noexcept
{
    return SerializationFormat::Xml;
}

std::string PugixmlEventSerializer::getLibraryName() const noexcept
{
    return "pugixml";
}

std::string PugixmlEventSerializer::getLibraryVersion() const noexcept
{
    return std::to_string(PUGIXML_VERSION / 1000) + "." +
           std::to_string((PUGIXML_VERSION % 1000) / 10) + "." +
           std::to_string(PUGIXML_VERSION % 10);
}

std::string PugixmlEventSerializer::getLastError() const
{
    return m_lastError;
}

bool PugixmlEventSerializer::supportsPrettyFormat() const noexcept
{
    return true;
}

void PugixmlEventSerializer::setLastError(const std::string& error) const
{
    m_lastError = error;
    spdlog::error("PugixmlEventSerializer: {}", error);
}

void PugixmlEventSerializer::eventToXml(const Event& event,
                                        pugi::xml_node& parent) const
{
    parent.append_attribute("timestamp") = event.getTimestampMs();

    switch (event.getType())
    {
    case EventType::MouseMove: {
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

    case EventType::MouseClick: {
        parent.append_attribute("type") = "mouse_click";
        const auto* data = event.getMouseData();
        auto pos = parent.append_child("Position");
        pos.append_attribute("x") = data->position.x;
        pos.append_attribute("y") = data->position.y;

        parent.append_attribute("button") = mouseButtonToString(data->button);

        if (static_cast<uint32_t>(data->modifiers) != 0)
        {
            parent.append_attribute("modifiers") =
                static_cast<uint32_t>(data->modifiers);
        }
        break;
    }

    case EventType::MouseDoubleClick: {
        parent.append_attribute("type") = "mouse_double_click";
        const auto* data = event.getMouseData();
        auto pos = parent.append_child("Position");
        pos.append_attribute("x") = data->position.x;
        pos.append_attribute("y") = data->position.y;

        parent.append_attribute("button") = mouseButtonToString(data->button);

        if (static_cast<uint32_t>(data->modifiers) != 0)
        {
            parent.append_attribute("modifiers") =
                static_cast<uint32_t>(data->modifiers);
        }
        break;
    }

    case EventType::MouseWheel: {
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

    case EventType::KeyPress: {
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

    case EventType::KeyRelease: {
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

    case EventType::KeyCombination: {
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

std::unique_ptr<Event> PugixmlEventSerializer::xmlToEvent(
    const pugi::xml_node& node) const
{
    try
    {
        std::string type = node.attribute("type").as_string();
        uint64_t timestampMs = node.attribute("timestamp").as_ullong();
        auto timePoint = Event::timestampFromMs(timestampMs);

        if (type == "mouse_move")
        {
            MouseEventData data;
            auto pos = node.child("Position");
            data.position.x = pos.attribute("x").as_int();
            data.position.y = pos.attribute("y").as_int();
            data.modifiers =
                static_cast<KeyModifier>(node.attribute("modifiers").as_uint());

            return std::make_unique<Event>(
                EventType::MouseMove, data, timePoint);
        }
        else if (type == "mouse_click")
        {
            MouseEventData data;
            auto pos = node.child("Position");
            data.position.x = pos.attribute("x").as_int();
            data.position.y = pos.attribute("y").as_int();
            data.button =
                stringToMouseButton(node.attribute("button").as_string());
            data.modifiers =
                static_cast<KeyModifier>(node.attribute("modifiers").as_uint());

            return std::make_unique<Event>(
                EventType::MouseClick, data, timePoint);
        }
        else if (type == "mouse_double_click")
        {
            MouseEventData data;
            auto pos = node.child("Position");
            data.position.x = pos.attribute("x").as_int();
            data.position.y = pos.attribute("y").as_int();
            data.button =
                stringToMouseButton(node.attribute("button").as_string());
            data.modifiers =
                static_cast<KeyModifier>(node.attribute("modifiers").as_uint());

            return std::make_unique<Event>(
                EventType::MouseDoubleClick, data, timePoint);
        }
        else if (type == "mouse_wheel")
        {
            MouseEventData data;
            auto pos = node.child("Position");
            data.position.x = pos.attribute("x").as_int();
            data.position.y = pos.attribute("y").as_int();
            data.wheelDelta = node.attribute("wheel_delta").as_int();
            data.modifiers =
                static_cast<KeyModifier>(node.attribute("modifiers").as_uint());

            return std::make_unique<Event>(
                EventType::MouseWheel, data, timePoint);
        }
        else if (type == "key_press")
        {
            KeyboardEventData data;
            data.keyCode = node.attribute("key_code").as_uint();
            data.keyName = node.attribute("key_name").as_string();
            data.modifiers =
                static_cast<KeyModifier>(node.attribute("modifiers").as_uint());
            data.isRepeated = node.attribute("repeated").as_bool();

            return std::make_unique<Event>(
                EventType::KeyPress, data, timePoint);
        }
        else if (type == "key_release")
        {
            KeyboardEventData data;
            data.keyCode = node.attribute("key_code").as_uint();
            data.keyName = node.attribute("key_name").as_string();
            data.modifiers =
                static_cast<KeyModifier>(node.attribute("modifiers").as_uint());

            return std::make_unique<Event>(
                EventType::KeyRelease, data, timePoint);
        }
        else if (type == "key_combination")
        {
            KeyboardEventData data;
            data.keyCode = node.attribute("key_code").as_uint();
            data.keyName = node.attribute("key_name").as_string();
            data.modifiers =
                static_cast<KeyModifier>(node.attribute("modifiers").as_uint());

            return std::make_unique<Event>(
                EventType::KeyCombination, data, timePoint);
        }

        return nullptr;
    }
    catch (const std::exception& e)
    {
        setLastError(std::string("Failed to convert XML to event: ") +
                     e.what());
        return nullptr;
    }
}

void PugixmlEventSerializer::metadataToXml(const StorageMetadata& metadata,
                                           pugi::xml_node& parent) const
{
    parent.append_attribute("version") = metadata.version.c_str();
    parent.append_attribute("application_name") =
        metadata.applicationName.c_str();
    parent.append_attribute("created_by") = metadata.createdBy.c_str();
    parent.append_attribute("description") = metadata.description.c_str();
    parent.append_attribute("creation_timestamp") = metadata.creationTimestamp;
    parent.append_attribute("total_duration_ms") = metadata.totalDurationMs;
    parent.append_attribute("total_events") = metadata.totalEvents;
    parent.append_attribute("platform") = metadata.platform.c_str();
    parent.append_attribute("screen_resolution") =
        metadata.screenResolution.c_str();
}

StorageMetadata PugixmlEventSerializer::xmlToMetadata(
    const pugi::xml_node& node) const
{
    StorageMetadata metadata;

    metadata.version = node.attribute("version").as_string();
    metadata.applicationName = node.attribute("application_name").as_string();
    metadata.createdBy = node.attribute("created_by").as_string();
    metadata.description = node.attribute("description").as_string();
    metadata.creationTimestamp =
        node.attribute("creation_timestamp").as_ullong();
    metadata.totalDurationMs = node.attribute("total_duration_ms").as_ullong();
    metadata.totalEvents = node.attribute("total_events").as_ullong();
    metadata.platform = node.attribute("platform").as_string();
    metadata.screenResolution = node.attribute("screen_resolution").as_string();

    return metadata;
}

const char* PugixmlEventSerializer::mouseButtonToString(
    MouseButton button) const
{
    switch (button)
    {
    case MouseButton::Left:
        return "left";
    case MouseButton::Right:
        return "right";
    case MouseButton::Middle:
        return "middle";
    case MouseButton::X1:
        return "x1";
    case MouseButton::X2:
        return "x2";
    default:
        return "left";
    }
}

MouseButton PugixmlEventSerializer::stringToMouseButton(
    const char* buttonStr) const
{
    if (strcmp(buttonStr, "left") == 0)
        return MouseButton::Left;
    else if (strcmp(buttonStr, "right") == 0)
        return MouseButton::Right;
    else if (strcmp(buttonStr, "middle") == 0)
        return MouseButton::Middle;
    else if (strcmp(buttonStr, "x1") == 0)
        return MouseButton::X1;
    else if (strcmp(buttonStr, "x2") == 0)
        return MouseButton::X2;
    else
        return MouseButton::Left; // Default fallback
}

} // namespace MouseRecorder::Core::Serialization
