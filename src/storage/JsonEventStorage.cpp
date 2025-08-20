// Copyright (c) 2025 JackLee
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "JsonEventStorage.hpp"
#include "core/Event.hpp"
#include <fstream>
#include "core/SpdlogConfig.hpp"

using json = nlohmann::json;

namespace MouseRecorder::Storage
{

JsonEventStorage::JsonEventStorage()
{
    spdlog::debug("JsonEventStorage: Constructor");
}

bool JsonEventStorage::saveEvents(
    const std::vector<std::unique_ptr<Core::Event>>& events,
    const std::string& filename,
    const Core::StorageMetadata& metadata)
{
    spdlog::info(
        "JsonEventStorage: Saving {} events to {}", events.size(), filename);

    try
    {
        json root;

        // Add metadata
        root["metadata"] = metadataToJson(metadata);

        // Add events
        json eventsArray = json::array();
        for (const auto& event : events)
        {
            if (event)
            {
                eventsArray.push_back(eventToJson(*event));
            }
        }
        root["events"] = eventsArray;

        // Write to file
        std::ofstream file(filename);
        if (!file.is_open())
        {
            setLastError("Failed to open file for writing: " + filename);
            return false;
        }

        file << root.dump(m_indentLevel);
        file.close();

        if (file.fail())
        {
            setLastError("Failed to write JSON data to file");
            return false;
        }

        spdlog::info("JsonEventStorage: Successfully saved {} events",
                     events.size());
        return true;
    }
    catch (const std::exception& e)
    {
        setLastError("JSON serialization error: " + std::string(e.what()));
        return false;
    }
}

bool JsonEventStorage::loadEvents(
    const std::string& filename,
    std::vector<std::unique_ptr<Core::Event>>& events,
    Core::StorageMetadata& metadata)
{
    spdlog::info("JsonEventStorage: Loading events from {}", filename);

    try
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            setLastError("Failed to open file for reading: " + filename);
            return false;
        }

        json root;
        file >> root;
        file.close();

        // Load metadata
        if (root.contains("metadata"))
        {
            metadata = jsonToMetadata(root["metadata"]);
        }

        // Load events
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

        spdlog::info("JsonEventStorage: Successfully loaded {} events",
                     events.size());
        return true;
    }
    catch (const std::exception& e)
    {
        setLastError("JSON deserialization error: " + std::string(e.what()));
        return false;
    }
}

Core::StorageFormat JsonEventStorage::getSupportedFormat() const noexcept
{
    return Core::StorageFormat::Json;
}

std::string JsonEventStorage::getFileExtension() const noexcept
{
    return ".json";
}

std::string JsonEventStorage::getFormatDescription() const noexcept
{
    return "JSON Event Recording";
}

bool JsonEventStorage::validateFile(const std::string& filename) const
{
    try
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            return false;
        }

        json root;
        file >> root;

        // Basic validation
        return root.is_object() && root.contains("metadata") &&
               root.contains("events") && root["events"].is_array();
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool JsonEventStorage::getFileMetadata(const std::string& filename,
                                       Core::StorageMetadata& metadata) const
{
    try
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            return false;
        }

        json root;
        file >> root;

        if (root.contains("metadata"))
        {
            metadata = jsonToMetadata(root["metadata"]);
            return true;
        }

        return false;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

std::string JsonEventStorage::getLastError() const
{
    return m_lastError;
}

void JsonEventStorage::setCompressionLevel(int level)
{
    // JSON doesn't support compression directly, but we can adjust indentation
    m_indentLevel = (level == 0) ? -1 : std::max(0, std::min(8, level));
}

bool JsonEventStorage::supportsCompression() const noexcept
{
    return false; // JSON itself doesn't support compression
}

json JsonEventStorage::eventToJson(const Core::Event& event) const
{
    json eventJson;

    eventJson["timestamp"] = event.getTimestampMs();

    switch (event.getType())
    {
    case Core::EventType::MouseMove:
        eventJson["type"] = "mouse_move";
        eventJson["data"] = mouseEventDataToJson(*event.getMouseData());
        break;

    case Core::EventType::MouseClick:
        eventJson["type"] = "mouse_click";
        eventJson["data"] = mouseEventDataToJson(*event.getMouseData());
        break;

    case Core::EventType::MouseDoubleClick:
        eventJson["type"] = "mouse_double_click";
        eventJson["data"] = mouseEventDataToJson(*event.getMouseData());
        break;

    case Core::EventType::MouseWheel:
        eventJson["type"] = "mouse_wheel";
        eventJson["data"] = mouseEventDataToJson(*event.getMouseData());
        break;

    case Core::EventType::KeyPress:
        eventJson["type"] = "key_press";
        eventJson["data"] = keyboardEventDataToJson(*event.getKeyboardData());
        break;

    case Core::EventType::KeyRelease:
        eventJson["type"] = "key_release";
        eventJson["data"] = keyboardEventDataToJson(*event.getKeyboardData());
        break;

    case Core::EventType::KeyCombination:
        eventJson["type"] = "key_combination";
        eventJson["data"] = keyboardEventDataToJson(*event.getKeyboardData());
        break;
    }

    return eventJson;
}

std::unique_ptr<Core::Event> JsonEventStorage::jsonToEvent(
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

        auto timePoint = Core::Event::timestampFromMs(timestamp);

        if (typeStr == "mouse_move")
        {
            auto mouseData = jsonToMouseEventData(eventJson["data"]);
            return std::make_unique<Core::Event>(
                Core::EventType::MouseMove, mouseData, timePoint);
        }
        if (typeStr == "mouse_click")
        {
            auto mouseData = jsonToMouseEventData(eventJson["data"]);
            return std::make_unique<Core::Event>(
                Core::EventType::MouseClick, mouseData, timePoint);
        }
        if (typeStr == "mouse_double_click")
        {
            auto mouseData = jsonToMouseEventData(eventJson["data"]);
            return std::make_unique<Core::Event>(
                Core::EventType::MouseDoubleClick, mouseData, timePoint);
        }
        if (typeStr == "mouse_wheel")
        {
            auto mouseData = jsonToMouseEventData(eventJson["data"]);
            return std::make_unique<Core::Event>(
                Core::EventType::MouseWheel, mouseData, timePoint);
        }
        if (typeStr == "key_press")
        {
            auto keyData = jsonToKeyboardEventData(eventJson["data"]);
            return std::make_unique<Core::Event>(
                Core::EventType::KeyPress, keyData, timePoint);
        }
        if (typeStr == "key_release")
        {
            auto keyData = jsonToKeyboardEventData(eventJson["data"]);
            return std::make_unique<Core::Event>(
                Core::EventType::KeyRelease, keyData, timePoint);
        }
        if (typeStr == "key_combination")
        {
            auto keyData = jsonToKeyboardEventData(eventJson["data"]);
            return std::make_unique<Core::Event>(
                Core::EventType::KeyCombination, keyData, timePoint);
        }

        return nullptr;
    }
    catch (const std::exception&)
    {
        return nullptr;
    }
}

json JsonEventStorage::metadataToJson(
    const Core::StorageMetadata& metadata) const
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

Core::StorageMetadata JsonEventStorage::jsonToMetadata(
    const json& metadataJson) const
{
    Core::StorageMetadata metadata;

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

json JsonEventStorage::mouseEventDataToJson(
    const Core::MouseEventData& data) const
{
    json dataJson;

    dataJson["position"] = {{"x", data.position.x}, {"y", data.position.y}};

    // Convert mouse button to string
    switch (data.button)
    {
    case Core::MouseButton::Left:
        dataJson["button"] = "left";
        break;
    case Core::MouseButton::Right:
        dataJson["button"] = "right";
        break;
    case Core::MouseButton::Middle:
        dataJson["button"] = "middle";
        break;
    case Core::MouseButton::X1:
        dataJson["button"] = "x1";
        break;
    case Core::MouseButton::X2:
        dataJson["button"] = "x2";
        break;
    }

    dataJson["wheel_delta"] = data.wheelDelta;
    dataJson["modifiers"] = static_cast<uint32_t>(data.modifiers);

    return dataJson;
}

Core::MouseEventData JsonEventStorage::jsonToMouseEventData(
    const json& dataJson) const
{
    Core::MouseEventData data;

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
            data.button = Core::MouseButton::Left;
        else if (buttonStr == "right")
            data.button = Core::MouseButton::Right;
        else if (buttonStr == "middle")
            data.button = Core::MouseButton::Middle;
        else if (buttonStr == "x1")
            data.button = Core::MouseButton::X1;
        else if (buttonStr == "x2")
            data.button = Core::MouseButton::X2;
    }

    if (dataJson.contains("wheel_delta"))
        data.wheelDelta = dataJson["wheel_delta"];
    if (dataJson.contains("modifiers"))
        data.modifiers = static_cast<Core::KeyModifier>(
            static_cast<uint32_t>(dataJson["modifiers"]));

    return data;
}

json JsonEventStorage::keyboardEventDataToJson(
    const Core::KeyboardEventData& data) const
{
    json dataJson;

    dataJson["key_code"] = data.keyCode;
    dataJson["key_name"] = data.keyName;
    dataJson["modifiers"] = static_cast<uint32_t>(data.modifiers);
    dataJson["is_repeated"] = data.isRepeated;

    return dataJson;
}

Core::KeyboardEventData JsonEventStorage::jsonToKeyboardEventData(
    const json& dataJson) const
{
    Core::KeyboardEventData data;

    if (dataJson.contains("key_code"))
        data.keyCode = dataJson["key_code"];
    if (dataJson.contains("key_name"))
        data.keyName = dataJson["key_name"].get<std::string>();
    if (dataJson.contains("modifiers"))
        data.modifiers = static_cast<Core::KeyModifier>(
            static_cast<uint32_t>(dataJson["modifiers"]));
    if (dataJson.contains("is_repeated"))
        data.isRepeated = dataJson["is_repeated"];

    return data;
}

void JsonEventStorage::setLastError(const std::string& error)
{
    m_lastError = error;
    spdlog::error("JsonEventStorage: {}", error);
}

} // namespace MouseRecorder::Storage
