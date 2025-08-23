// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "NlohmannJsonEventSerializer.hpp"
#include "core/Event.hpp"
#include "core/SpdlogConfig.hpp"

using json = nlohmann::json;

namespace MouseRecorder::Core::Serialization
{

NlohmannJsonEventSerializer::NlohmannJsonEventSerializer()
{
    spdlog::debug("NlohmannJsonEventSerializer: Constructor");
}

std::string NlohmannJsonEventSerializer::serializeEvents(
    const std::vector<std::unique_ptr<Event>>& events,
    const StorageMetadata& metadata,
    bool prettyFormat) const
{
    try
    {
        json root;

        // Add metadata
        root["metadata"] = metadataToJson(metadata);

        // Add events array
        json eventsArray = json::array();
        for (const auto& event : events)
        {
            if (event)
            {
                eventsArray.push_back(eventToJson(*event));
            }
        }
        root["events"] = eventsArray;

        // Convert to string
        const int indent = prettyFormat ? m_indentLevel : -1;
        return root.dump(indent);
    }
    catch (const std::exception& e)
    {
        setLastError(std::string("Failed to serialize events: ") + e.what());
        return "";
    }
}

bool NlohmannJsonEventSerializer::deserializeEvents(
    const std::string& data,
    std::vector<std::unique_ptr<Event>>& events,
    StorageMetadata& metadata) const
{
    try
    {
        json root = json::parse(data);

        if (!root.is_object())
        {
            setLastError("Root JSON element is not an object");
            return false;
        }

        // Parse metadata
        if (root.contains("metadata") && root["metadata"].is_object())
        {
            metadata = jsonToMetadata(root["metadata"]);
        }

        // Parse events
        events.clear();
        if (root.contains("events") && root["events"].is_array())
        {
            for (const auto& eventJson : root["events"])
            {
                auto event = jsonToEvent(eventJson);
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

std::string NlohmannJsonEventSerializer::serializeMetadata(
    const StorageMetadata& metadata, bool prettyFormat) const
{
    try
    {
        json metadataJson = metadataToJson(metadata);
        const int indent = prettyFormat ? m_indentLevel : -1;
        return metadataJson.dump(indent);
    }
    catch (const std::exception& e)
    {
        setLastError(std::string("Failed to serialize metadata: ") + e.what());
        return "";
    }
}

bool NlohmannJsonEventSerializer::deserializeMetadata(
    const std::string& data, StorageMetadata& metadata) const
{
    try
    {
        json metadataJson = json::parse(data);

        if (!metadataJson.is_object())
        {
            setLastError("Root JSON element is not an object");
            return false;
        }

        metadata = jsonToMetadata(metadataJson);
        return true;
    }
    catch (const std::exception& e)
    {
        setLastError(std::string("Failed to deserialize metadata: ") +
                     e.what());
        return false;
    }
}

bool NlohmannJsonEventSerializer::validateFormat(const std::string& data) const
{
    try
    {
        [[maybe_unused]] auto parsed = json::parse(data);
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

SerializationFormat NlohmannJsonEventSerializer::getSupportedFormat()
    const noexcept
{
    return SerializationFormat::Json;
}

std::string NlohmannJsonEventSerializer::getLibraryName() const noexcept
{
    return "nlohmann_json";
}

std::string NlohmannJsonEventSerializer::getLibraryVersion() const noexcept
{
    return std::to_string(NLOHMANN_JSON_VERSION_MAJOR) + "." +
           std::to_string(NLOHMANN_JSON_VERSION_MINOR) + "." +
           std::to_string(NLOHMANN_JSON_VERSION_PATCH);
}

std::string NlohmannJsonEventSerializer::getLastError() const
{
    return m_lastError;
}

bool NlohmannJsonEventSerializer::supportsPrettyFormat() const noexcept
{
    return true;
}

void NlohmannJsonEventSerializer::setLastError(const std::string& error) const
{
    m_lastError = error;
    spdlog::error("NlohmannJsonEventSerializer: {}", error);
}

json NlohmannJsonEventSerializer::eventToJson(const Event& event) const
{
    json eventJson;

    // Convert event type to string
    switch (event.getType())
    {
    case EventType::MouseMove:
        eventJson["type"] = "mouse_move";
        break;
    case EventType::MouseClick:
        eventJson["type"] = "mouse_click";
        break;
    case EventType::MouseDoubleClick:
        eventJson["type"] = "mouse_double_click";
        break;
    case EventType::MouseWheel:
        eventJson["type"] = "mouse_wheel";
        break;
    case EventType::KeyPress:
        eventJson["type"] = "key_press";
        break;
    case EventType::KeyRelease:
        eventJson["type"] = "key_release";
        break;
    case EventType::KeyCombination:
        eventJson["type"] = "key_combination";
        break;
    }

    eventJson["timestamp"] = event.getTimestampMs();

    // Add type-specific data
    if (auto mouseData = event.getMouseData())
    {
        eventJson["data"] = mouseEventDataToJson(*mouseData);
    }
    else if (auto keyboardData = event.getKeyboardData())
    {
        eventJson["data"] = keyboardEventDataToJson(*keyboardData);
    }

    return eventJson;
}

std::unique_ptr<Event> NlohmannJsonEventSerializer::jsonToEvent(
    const json& eventJson) const
{
    try
    {
        if (!eventJson.contains("type") || !eventJson.contains("timestamp") ||
            !eventJson.contains("data"))
        {
            return nullptr;
        }

        auto typeStr = eventJson["type"];
        uint64_t timestamp = eventJson["timestamp"];
        auto timePoint = Event::timestampFromMs(timestamp);

        if (typeStr == "mouse_move")
        {
            auto mouseData = jsonToMouseEventData(eventJson["data"]);
            return std::make_unique<Event>(
                EventType::MouseMove, mouseData, timePoint);
        }
        if (typeStr == "mouse_click")
        {
            auto mouseData = jsonToMouseEventData(eventJson["data"]);
            return std::make_unique<Event>(
                EventType::MouseClick, mouseData, timePoint);
        }
        if (typeStr == "mouse_double_click")
        {
            auto mouseData = jsonToMouseEventData(eventJson["data"]);
            return std::make_unique<Event>(
                EventType::MouseDoubleClick, mouseData, timePoint);
        }
        if (typeStr == "mouse_wheel")
        {
            auto mouseData = jsonToMouseEventData(eventJson["data"]);
            return std::make_unique<Event>(
                EventType::MouseWheel, mouseData, timePoint);
        }
        if (typeStr == "key_press")
        {
            auto keyData = jsonToKeyboardEventData(eventJson["data"]);
            return std::make_unique<Event>(
                EventType::KeyPress, keyData, timePoint);
        }
        if (typeStr == "key_release")
        {
            auto keyData = jsonToKeyboardEventData(eventJson["data"]);
            return std::make_unique<Event>(
                EventType::KeyRelease, keyData, timePoint);
        }
        if (typeStr == "key_combination")
        {
            auto keyData = jsonToKeyboardEventData(eventJson["data"]);
            return std::make_unique<Event>(
                EventType::KeyCombination, keyData, timePoint);
        }

        return nullptr;
    }
    catch (const std::exception&)
    {
        return nullptr;
    }
}

json NlohmannJsonEventSerializer::metadataToJson(
    const StorageMetadata& metadata) const
{
    json metadataJson;

    metadataJson["version"] = metadata.version;
    metadataJson["application_name"] = metadata.applicationName;
    metadataJson["created_by"] = metadata.createdBy;
    metadataJson["description"] = metadata.description;
    metadataJson["creation_timestamp"] = metadata.creationTimestamp;
    metadataJson["total_duration_ms"] = metadata.totalDurationMs;
    metadataJson["total_events"] = metadata.totalEvents;
    metadataJson["platform"] = metadata.platform;
    metadataJson["screen_resolution"] = metadata.screenResolution;

    return metadataJson;
}

StorageMetadata NlohmannJsonEventSerializer::jsonToMetadata(
    const json& metadataJson) const
{
    StorageMetadata metadata;

    if (metadataJson.contains("version"))
        metadata.version = metadataJson["version"].get<std::string>();
    if (metadataJson.contains("application_name"))
        metadata.applicationName =
            metadataJson["application_name"].get<std::string>();
    if (metadataJson.contains("created_by"))
        metadata.createdBy = metadataJson["created_by"].get<std::string>();
    if (metadataJson.contains("description"))
        metadata.description = metadataJson["description"].get<std::string>();
    if (metadataJson.contains("creation_timestamp"))
        metadata.creationTimestamp = metadataJson["creation_timestamp"];
    if (metadataJson.contains("total_duration_ms"))
        metadata.totalDurationMs = metadataJson["total_duration_ms"];
    if (metadataJson.contains("total_events"))
        metadata.totalEvents = metadataJson["total_events"];
    if (metadataJson.contains("platform"))
        metadata.platform = metadataJson["platform"].get<std::string>();
    if (metadataJson.contains("screen_resolution"))
        metadata.screenResolution =
            metadataJson["screen_resolution"].get<std::string>();

    return metadata;
}

json NlohmannJsonEventSerializer::mouseEventDataToJson(
    const MouseEventData& data) const
{
    json dataJson;

    dataJson["position"] = {{"x", data.position.x}, {"y", data.position.y}};

    // Convert mouse button to string
    switch (data.button)
    {
    case MouseButton::Left:
        dataJson["button"] = "left";
        break;
    case MouseButton::Right:
        dataJson["button"] = "right";
        break;
    case MouseButton::Middle:
        dataJson["button"] = "middle";
        break;
    case MouseButton::X1:
        dataJson["button"] = "x1";
        break;
    case MouseButton::X2:
        dataJson["button"] = "x2";
        break;
    }

    dataJson["wheel_delta"] = data.wheelDelta;
    dataJson["modifiers"] = static_cast<uint32_t>(data.modifiers);

    return dataJson;
}

MouseEventData NlohmannJsonEventSerializer::jsonToMouseEventData(
    const json& dataJson) const
{
    MouseEventData data;

    if (dataJson.contains("position"))
    {
        auto pos = dataJson["position"];
        if (pos.contains("x"))
            data.position.x = pos["x"];
        if (pos.contains("y"))
            data.position.y = pos["y"];
    }

    if (dataJson.contains("button"))
    {
        if (auto buttonStr = dataJson["button"]; buttonStr == "left")
            data.button = MouseButton::Left;
        else if (buttonStr == "right")
            data.button = MouseButton::Right;
        else if (buttonStr == "middle")
            data.button = MouseButton::Middle;
        else if (buttonStr == "x1")
            data.button = MouseButton::X1;
        else if (buttonStr == "x2")
            data.button = MouseButton::X2;
    }

    if (dataJson.contains("wheel_delta"))
        data.wheelDelta = dataJson["wheel_delta"];
    if (dataJson.contains("modifiers"))
        data.modifiers = static_cast<KeyModifier>(
            static_cast<uint32_t>(dataJson["modifiers"]));

    return data;
}

json NlohmannJsonEventSerializer::keyboardEventDataToJson(
    const KeyboardEventData& data) const
{
    json dataJson;

    dataJson["key_code"] = data.keyCode;
    dataJson["key_name"] = data.keyName;
    dataJson["modifiers"] = static_cast<uint32_t>(data.modifiers);
    dataJson["is_repeated"] = data.isRepeated;

    return dataJson;
}

KeyboardEventData NlohmannJsonEventSerializer::jsonToKeyboardEventData(
    const json& dataJson) const
{
    KeyboardEventData data;

    if (dataJson.contains("key_code"))
        data.keyCode = dataJson["key_code"];
    if (dataJson.contains("key_name"))
        data.keyName = dataJson["key_name"].get<std::string>();
    if (dataJson.contains("modifiers"))
        data.modifiers = static_cast<KeyModifier>(
            static_cast<uint32_t>(dataJson["modifiers"]));
    if (dataJson.contains("is_repeated"))
        data.isRepeated = dataJson["is_repeated"];

    return data;
}

} // namespace MouseRecorder::Core::Serialization
