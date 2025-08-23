// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "QtJsonEventSerializer.hpp"
#include "core/Event.hpp"
#include "core/SpdlogConfig.hpp"
#include <QJsonParseError>
#include <QVersionNumber>

namespace MouseRecorder::Core::Serialization
{

QtJsonEventSerializer::QtJsonEventSerializer()
{
    spdlog::debug("QtJsonEventSerializer: Constructor");
}

std::string QtJsonEventSerializer::serializeEvents(
    const std::vector<std::unique_ptr<Event>>& events,
    const StorageMetadata& metadata,
    bool prettyFormat) const
{
    try
    {
        QJsonObject root;

        // Add metadata
        root["metadata"] = metadataToJson(metadata);

        // Add events array
        QJsonArray eventsArray;
        for (const auto& event : events)
        {
            if (event)
            {
                eventsArray.append(eventToJson(*event));
            }
        }
        root["events"] = eventsArray;

        QJsonDocument doc(root);
        QByteArray result = prettyFormat ? doc.toJson(QJsonDocument::Indented)
                                         : doc.toJson(QJsonDocument::Compact);

        return result.toStdString();
    }
    catch (const std::exception& e)
    {
        setLastError(std::string("Failed to serialize events: ") + e.what());
        return "";
    }
}

bool QtJsonEventSerializer::deserializeEvents(
    const std::string& data,
    std::vector<std::unique_ptr<Event>>& events,
    StorageMetadata& metadata) const
{
    try
    {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(
            QByteArray::fromStdString(data), &parseError);

        if (parseError.error != QJsonParseError::NoError)
        {
            setLastError("JSON parse error: " +
                         parseError.errorString().toStdString());
            return false;
        }

        if (!doc.isObject())
        {
            setLastError("Root JSON element is not an object");
            return false;
        }

        QJsonObject root = doc.object();

        // Parse metadata
        if (root.contains("metadata") && root["metadata"].isObject())
        {
            metadata = jsonToMetadata(root["metadata"].toObject());
        }

        // Parse events
        events.clear();
        if (root.contains("events") && root["events"].isArray())
        {
            QJsonArray eventsArray = root["events"].toArray();
            events.reserve(eventsArray.size());

            for (const auto& value : eventsArray)
            {
                if (value.isObject())
                {
                    auto event = jsonToEvent(value.toObject());
                    if (event)
                    {
                        events.push_back(std::move(event));
                    }
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

std::string QtJsonEventSerializer::serializeMetadata(
    const StorageMetadata& metadata, bool prettyFormat) const
{
    try
    {
        QJsonObject metadataJson = metadataToJson(metadata);
        QJsonDocument doc(metadataJson);
        QByteArray result = prettyFormat ? doc.toJson(QJsonDocument::Indented)
                                         : doc.toJson(QJsonDocument::Compact);

        return result.toStdString();
    }
    catch (const std::exception& e)
    {
        setLastError(std::string("Failed to serialize metadata: ") + e.what());
        return "";
    }
}

bool QtJsonEventSerializer::deserializeMetadata(const std::string& data,
                                                StorageMetadata& metadata) const
{
    try
    {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(
            QByteArray::fromStdString(data), &parseError);

        if (parseError.error != QJsonParseError::NoError)
        {
            setLastError("JSON parse error: " +
                         parseError.errorString().toStdString());
            return false;
        }

        if (!doc.isObject())
        {
            setLastError("Root JSON element is not an object");
            return false;
        }

        metadata = jsonToMetadata(doc.object());
        return true;
    }
    catch (const std::exception& e)
    {
        setLastError(std::string("Failed to deserialize metadata: ") +
                     e.what());
        return false;
    }
}

bool QtJsonEventSerializer::validateFormat(const std::string& data) const
{
    QJsonParseError parseError;
    QJsonDocument doc =
        QJsonDocument::fromJson(QByteArray::fromStdString(data), &parseError);

    return parseError.error == QJsonParseError::NoError && doc.isObject();
}

SerializationFormat QtJsonEventSerializer::getSupportedFormat() const noexcept
{
    return SerializationFormat::Json;
}

std::string QtJsonEventSerializer::getLibraryName() const noexcept
{
    return "Qt";
}

std::string QtJsonEventSerializer::getLibraryVersion() const noexcept
{
    return QVersionNumber::fromString(qVersion()).toString().toStdString();
}

std::string QtJsonEventSerializer::getLastError() const
{
    return m_lastError;
}

bool QtJsonEventSerializer::supportsPrettyFormat() const noexcept
{
    return true;
}

void QtJsonEventSerializer::setLastError(const std::string& error) const
{
    m_lastError = error;
    spdlog::error("QtJsonEventSerializer: {}", error);
}

QJsonObject QtJsonEventSerializer::eventToJson(const Event& event) const
{
    QJsonObject eventJson;

    // Common fields
    eventJson["type"] = eventTypeToString(event.getType());
    eventJson["timestamp"] = static_cast<qint64>(event.getTimestampMs());

    // Type-specific data
    QJsonObject dataJson;

    if (auto mouseData = event.getMouseData())
    {
        dataJson["position"] = pointToJson(mouseData->position);
        dataJson["button"] = static_cast<int>(mouseData->button);
        dataJson["wheel_delta"] = mouseData->wheelDelta;
        dataJson["modifiers"] = static_cast<qint64>(mouseData->modifiers);
    }
    else if (auto keyboardData = event.getKeyboardData())
    {
        dataJson["key_code"] = static_cast<qint64>(keyboardData->keyCode);
        dataJson["key_name"] = QString::fromStdString(keyboardData->keyName);
        dataJson["modifiers"] = static_cast<qint64>(keyboardData->modifiers);
        dataJson["is_repeated"] = keyboardData->isRepeated;
    }

    eventJson["data"] = dataJson;

    return eventJson;
}

std::unique_ptr<Event> QtJsonEventSerializer::jsonToEvent(
    const QJsonObject& json) const
{
    try
    {
        // Parse common fields
        QString typeStr = json["type"].toString();
        EventType type = stringToEventType(typeStr);

        uint64_t timestampMs = json["timestamp"].toVariant().toLongLong();
        auto timePoint = Event::timestampFromMs(timestampMs);

        QJsonObject dataJson = json["data"].toObject();

        // Create event based on type
        if (type == EventType::MouseMove || type == EventType::MouseClick ||
            type == EventType::MouseDoubleClick ||
            type == EventType::MouseWheel)
        {
            MouseEventData mouseData;
            mouseData.position = jsonToPoint(dataJson["position"].toObject());
            mouseData.button =
                static_cast<MouseButton>(dataJson["button"].toInt());
            mouseData.wheelDelta = dataJson["wheel_delta"].toInt();
            mouseData.modifiers =
                static_cast<KeyModifier>(dataJson["modifiers"].toInt());

            return std::make_unique<Event>(type, mouseData, timePoint);
        }
        else if (type == EventType::KeyPress || type == EventType::KeyRelease ||
                 type == EventType::KeyCombination)
        {
            KeyboardEventData keyData;
            keyData.keyCode =
                static_cast<uint32_t>(dataJson["key_code"].toInt());
            keyData.keyName = dataJson["key_name"].toString().toStdString();
            keyData.modifiers =
                static_cast<KeyModifier>(dataJson["modifiers"].toInt());
            keyData.isRepeated = dataJson["is_repeated"].toBool();

            return std::make_unique<Event>(type, keyData, timePoint);
        }

        return nullptr;
    }
    catch (const std::exception& e)
    {
        setLastError(std::string("Failed to convert JSON to event: ") +
                     e.what());
        return nullptr;
    }
}

QJsonObject QtJsonEventSerializer::metadataToJson(
    const StorageMetadata& metadata) const
{
    QJsonObject metadataJson;

    metadataJson["version"] = QString::fromStdString(metadata.version);
    metadataJson["application_name"] =
        QString::fromStdString(metadata.applicationName);
    metadataJson["created_by"] = QString::fromStdString(metadata.createdBy);
    metadataJson["description"] = QString::fromStdString(metadata.description);
    metadataJson["creation_timestamp"] =
        static_cast<qint64>(metadata.creationTimestamp);
    metadataJson["total_duration_ms"] =
        static_cast<qint64>(metadata.totalDurationMs);
    metadataJson["total_events"] = static_cast<qint64>(metadata.totalEvents);
    metadataJson["platform"] = QString::fromStdString(metadata.platform);
    metadataJson["screen_resolution"] =
        QString::fromStdString(metadata.screenResolution);

    return metadataJson;
}

StorageMetadata QtJsonEventSerializer::jsonToMetadata(
    const QJsonObject& json) const
{
    StorageMetadata metadata;

    metadata.version = json["version"].toString().toStdString();
    metadata.applicationName =
        json["application_name"].toString().toStdString();
    metadata.createdBy = json["created_by"].toString().toStdString();
    metadata.description = json["description"].toString().toStdString();
    metadata.creationTimestamp =
        json["creation_timestamp"].toVariant().toLongLong();
    metadata.totalDurationMs =
        json["total_duration_ms"].toVariant().toLongLong();
    metadata.totalEvents = json["total_events"].toVariant().toLongLong();
    metadata.platform = json["platform"].toString().toStdString();
    metadata.screenResolution =
        json["screen_resolution"].toString().toStdString();

    return metadata;
}

QString QtJsonEventSerializer::eventTypeToString(EventType type) const
{
    switch (type)
    {
    case EventType::MouseMove:
        return "mouse_move";
    case EventType::MouseClick:
        return "mouse_click";
    case EventType::MouseDoubleClick:
        return "mouse_double_click";
    case EventType::MouseWheel:
        return "mouse_wheel";
    case EventType::KeyPress:
        return "key_press";
    case EventType::KeyRelease:
        return "key_release";
    case EventType::KeyCombination:
        return "key_combination";
    default:
        return "unknown";
    }
}

EventType QtJsonEventSerializer::stringToEventType(const QString& typeStr) const
{
    if (typeStr == "mouse_move")
        return EventType::MouseMove;
    else if (typeStr == "mouse_click")
        return EventType::MouseClick;
    else if (typeStr == "mouse_double_click")
        return EventType::MouseDoubleClick;
    else if (typeStr == "mouse_wheel")
        return EventType::MouseWheel;
    else if (typeStr == "key_press")
        return EventType::KeyPress;
    else if (typeStr == "key_release")
        return EventType::KeyRelease;
    else if (typeStr == "key_combination")
        return EventType::KeyCombination;
    else
        return EventType::MouseMove; // Default fallback
}

QJsonObject QtJsonEventSerializer::pointToJson(const Point& point) const
{
    QJsonObject pointJson;
    pointJson["x"] = point.x;
    pointJson["y"] = point.y;
    return pointJson;
}

Point QtJsonEventSerializer::jsonToPoint(const QJsonObject& json) const
{
    Point point;
    point.x = json["x"].toInt();
    point.y = json["y"].toInt();
    return point;
}

} // namespace MouseRecorder::Core::Serialization
