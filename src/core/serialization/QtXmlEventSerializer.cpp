// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "QtXmlEventSerializer.hpp"
#include "core/Event.hpp"
#include "core/SpdlogConfig.hpp"
#include <QVersionNumber>

namespace MouseRecorder::Core::Serialization
{

QtXmlEventSerializer::QtXmlEventSerializer()
{
    spdlog::debug("QtXmlEventSerializer: Constructor");
}

std::string QtXmlEventSerializer::serializeEvents(
    const std::vector<std::unique_ptr<Event>>& events,
    const StorageMetadata& metadata,
    bool prettyFormat) const
{
    try
    {
        QDomDocument doc;

        // Create root element
        QDomElement root = doc.createElement("mouserecorder");
        doc.appendChild(root);

        // Add metadata
        QDomElement metadataElement = metadataToXml(doc, metadata);
        root.appendChild(metadataElement);

        // Add events
        QDomElement eventsElement = doc.createElement("events");
        setXmlAttribute(
            eventsElement, "count", static_cast<qint64>(events.size()));

        for (const auto& event : events)
        {
            if (event)
            {
                QDomElement eventElement = eventToXml(doc, *event);
                eventsElement.appendChild(eventElement);
            }
        }
        root.appendChild(eventsElement);

        // Convert to string
        const int indent = prettyFormat ? 2 : -1;
        return doc.toString(indent).toStdString();
    }
    catch (const std::exception& e)
    {
        setLastError(std::string("Failed to serialize events: ") + e.what());
        return "";
    }
}

bool QtXmlEventSerializer::deserializeEvents(
    const std::string& data,
    std::vector<std::unique_ptr<Event>>& events,
    StorageMetadata& metadata) const
{
    try
    {
        QDomDocument doc;
        QString errorMsg;
        int errorLine, errorColumn;

        if (!doc.setContent(QString::fromStdString(data),
                            &errorMsg,
                            &errorLine,
                            &errorColumn))
        {
            setLastError(QString("XML parse error at line %1, column %2: %3")
                             .arg(errorLine)
                             .arg(errorColumn)
                             .arg(errorMsg)
                             .toStdString());
            return false;
        }

        QDomElement root = doc.documentElement();
        if (root.tagName() != "mouserecorder")
        {
            setLastError("Invalid XML: root element should be 'mouserecorder'");
            return false;
        }

        // Parse metadata
        QDomElement metadataElement = root.firstChildElement("metadata");
        if (!metadataElement.isNull())
        {
            metadata = xmlToMetadata(metadataElement);
        }

        // Parse events
        events.clear();
        QDomElement eventsElement = root.firstChildElement("events");
        if (!eventsElement.isNull())
        {
            QDomElement eventElement = eventsElement.firstChildElement("event");
            while (!eventElement.isNull())
            {
                auto event = xmlToEvent(eventElement);
                if (event)
                {
                    events.push_back(std::move(event));
                }
                eventElement = eventElement.nextSiblingElement("event");
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

std::string QtXmlEventSerializer::serializeMetadata(
    const StorageMetadata& metadata, bool prettyFormat) const
{
    try
    {
        QDomDocument doc;
        QDomElement metadataElement = metadataToXml(doc, metadata);
        doc.appendChild(metadataElement);

        const int indent = prettyFormat ? 2 : -1;
        return doc.toString(indent).toStdString();
    }
    catch (const std::exception& e)
    {
        setLastError(std::string("Failed to serialize metadata: ") + e.what());
        return "";
    }
}

bool QtXmlEventSerializer::deserializeMetadata(const std::string& data,
                                               StorageMetadata& metadata) const
{
    try
    {
        QDomDocument doc;
        QString errorMsg;
        int errorLine, errorColumn;

        if (!doc.setContent(QString::fromStdString(data),
                            &errorMsg,
                            &errorLine,
                            &errorColumn))
        {
            setLastError(QString("XML parse error at line %1, column %2: %3")
                             .arg(errorLine)
                             .arg(errorColumn)
                             .arg(errorMsg)
                             .toStdString());
            return false;
        }

        QDomElement root = doc.documentElement();
        metadata = xmlToMetadata(root);
        return true;
    }
    catch (const std::exception& e)
    {
        setLastError(std::string("Failed to deserialize metadata: ") +
                     e.what());
        return false;
    }
}

bool QtXmlEventSerializer::validateFormat(const std::string& data) const
{
    QDomDocument doc;
    QString errorMsg;
    int errorLine, errorColumn;

    return doc.setContent(
        QString::fromStdString(data), &errorMsg, &errorLine, &errorColumn);
}

SerializationFormat QtXmlEventSerializer::getSupportedFormat() const noexcept
{
    return SerializationFormat::Xml;
}

std::string QtXmlEventSerializer::getLibraryName() const noexcept
{
    return "Qt";
}

std::string QtXmlEventSerializer::getLibraryVersion() const noexcept
{
    return QVersionNumber::fromString(qVersion()).toString().toStdString();
}

std::string QtXmlEventSerializer::getLastError() const
{
    return m_lastError;
}

bool QtXmlEventSerializer::supportsPrettyFormat() const noexcept
{
    return true;
}

void QtXmlEventSerializer::setLastError(const std::string& error) const
{
    m_lastError = error;
    spdlog::error("QtXmlEventSerializer: {}", error);
}

QDomElement QtXmlEventSerializer::eventToXml(QDomDocument& doc,
                                             const Event& event) const
{
    QDomElement eventElement = doc.createElement("event");

    // Common attributes
    setXmlAttribute(eventElement, "type", eventTypeToString(event.getType()));
    setXmlAttribute(
        eventElement, "timestamp", static_cast<qint64>(event.getTimestampMs()));

    // Type-specific data
    if (auto mouseData = event.getMouseData())
    {
        QDomElement positionElement =
            pointToXml(doc, mouseData->position, "position");
        eventElement.appendChild(positionElement);

        setXmlAttribute(
            eventElement, "button", static_cast<int>(mouseData->button));
        setXmlAttribute(eventElement, "wheel_delta", mouseData->wheelDelta);
        setXmlAttribute(eventElement,
                        "modifiers",
                        static_cast<uint32_t>(mouseData->modifiers));
    }
    else if (auto keyboardData = event.getKeyboardData())
    {
        setXmlAttribute(eventElement, "key_code", keyboardData->keyCode);
        setXmlAttribute(eventElement,
                        "modifiers",
                        static_cast<uint32_t>(keyboardData->modifiers));
        setXmlAttribute(eventElement, "is_repeated", keyboardData->isRepeated);

        if (!keyboardData->keyName.empty())
        {
            QDomElement keyNameElement = doc.createElement("key_name");
            keyNameElement.appendChild(doc.createTextNode(
                QString::fromStdString(keyboardData->keyName)));
            eventElement.appendChild(keyNameElement);
        }
    }

    return eventElement;
}

std::unique_ptr<Event> QtXmlEventSerializer::xmlToEvent(
    const QDomElement& element) const
{
    try
    {
        // Parse common attributes
        QString typeStr = element.attribute("type");
        EventType type = stringToEventType(typeStr);

        qint64 timestampMs = getXmlAttribute<qint64>(element, "timestamp", 0);
        auto timePoint = Event::timestampFromMs(timestampMs);

        // Create event based on type
        if (type == EventType::MouseMove || type == EventType::MouseClick ||
            type == EventType::MouseDoubleClick ||
            type == EventType::MouseWheel)
        {
            MouseEventData mouseData;

            QDomElement positionElement = element.firstChildElement("position");
            mouseData.position = xmlToPoint(positionElement);

            mouseData.button = static_cast<MouseButton>(
                getXmlAttribute<int>(element, "button", 0));
            mouseData.wheelDelta =
                getXmlAttribute<int>(element, "wheel_delta", 0);
            mouseData.modifiers = static_cast<KeyModifier>(
                getXmlAttribute<uint32_t>(element, "modifiers", 0));

            return std::make_unique<Event>(type, mouseData, timePoint);
        }
        if (type == EventType::KeyPress || type == EventType::KeyRelease ||
            type == EventType::KeyCombination)
        {
            KeyboardEventData keyData;

            keyData.keyCode = getXmlAttribute<uint32_t>(element, "key_code", 0);
            keyData.modifiers = static_cast<KeyModifier>(
                getXmlAttribute<uint32_t>(element, "modifiers", 0));
            keyData.isRepeated =
                getXmlAttribute<bool>(element, "is_repeated", false);

            if (QDomElement keyNameElement =
                    element.firstChildElement("key_name");
                !keyNameElement.isNull())
            {
                keyData.keyName = keyNameElement.text().toStdString();
            }

            return std::make_unique<Event>(type, keyData, timePoint);
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

QDomElement QtXmlEventSerializer::metadataToXml(
    QDomDocument& doc, const StorageMetadata& metadata) const
{
    QDomElement metadataElement = doc.createElement("metadata");

    setXmlAttribute(
        metadataElement, "version", QString::fromStdString(metadata.version));
    setXmlAttribute(metadataElement,
                    "application_name",
                    QString::fromStdString(metadata.applicationName));
    setXmlAttribute(metadataElement,
                    "created_by",
                    QString::fromStdString(metadata.createdBy));
    setXmlAttribute(metadataElement,
                    "description",
                    QString::fromStdString(metadata.description));
    setXmlAttribute(metadataElement,
                    "creation_timestamp",
                    static_cast<qint64>(metadata.creationTimestamp));
    setXmlAttribute(metadataElement,
                    "total_duration_ms",
                    static_cast<qint64>(metadata.totalDurationMs));
    setXmlAttribute(metadataElement,
                    "total_events",
                    static_cast<qint64>(metadata.totalEvents));
    setXmlAttribute(
        metadataElement, "platform", QString::fromStdString(metadata.platform));
    setXmlAttribute(metadataElement,
                    "screen_resolution",
                    QString::fromStdString(metadata.screenResolution));

    return metadataElement;
}

StorageMetadata QtXmlEventSerializer::xmlToMetadata(
    const QDomElement& element) const
{
    StorageMetadata metadata;

    metadata.version = element.attribute("version").toStdString();
    metadata.applicationName =
        element.attribute("application_name").toStdString();
    metadata.createdBy = element.attribute("created_by").toStdString();
    metadata.description = element.attribute("description").toStdString();
    metadata.creationTimestamp =
        getXmlAttribute<qint64>(element, "creation_timestamp", 0);
    metadata.totalDurationMs =
        getXmlAttribute<qint64>(element, "total_duration_ms", 0);
    metadata.totalEvents = getXmlAttribute<qint64>(element, "total_events", 0);
    metadata.platform = element.attribute("platform").toStdString();
    metadata.screenResolution =
        element.attribute("screen_resolution").toStdString();

    return metadata;
}

QString QtXmlEventSerializer::eventTypeToString(EventType type) const
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

EventType QtXmlEventSerializer::stringToEventType(const QString& typeStr) const
{
    if (typeStr == "mouse_move")
        return EventType::MouseMove;
    if (typeStr == "mouse_click")
        return EventType::MouseClick;
    if (typeStr == "mouse_double_click")
        return EventType::MouseDoubleClick;
    if (typeStr == "mouse_wheel")
        return EventType::MouseWheel;
    if (typeStr == "key_press")
        return EventType::KeyPress;
    if (typeStr == "key_release")
        return EventType::KeyRelease;
    if (typeStr == "key_combination")
        return EventType::KeyCombination;
    return EventType::MouseMove; // Default fallback
}

QDomElement QtXmlEventSerializer::pointToXml(QDomDocument& doc,
                                             const Point& point,
                                             const QString& elementName) const
{
    QDomElement pointElement = doc.createElement(elementName);
    setXmlAttribute(pointElement, "x", point.x);
    setXmlAttribute(pointElement, "y", point.y);
    return pointElement;
}

Point QtXmlEventSerializer::xmlToPoint(const QDomElement& element) const
{
    Point point;
    point.x = getXmlAttribute<int>(element, "x", 0);
    point.y = getXmlAttribute<int>(element, "y", 0);
    return point;
}

template <typename T>
void QtXmlEventSerializer::setXmlAttribute(QDomElement& element,
                                           const QString& name,
                                           const T& value) const
{
    if constexpr (std::is_same_v<T, QString>)
    {
        element.setAttribute(name, value);
    }
    else if constexpr (std::is_same_v<T, std::string>)
    {
        element.setAttribute(name, QString::fromStdString(value));
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        element.setAttribute(name, value ? "true" : "false");
    }
    else if constexpr (std::is_arithmetic_v<T>)
    {
        element.setAttribute(name, QString::number(value));
    }
    else
    {
        element.setAttribute(name, QString::number(static_cast<qint64>(value)));
    }
}

template <typename T>
T QtXmlEventSerializer::getXmlAttribute(const QDomElement& element,
                                        const QString& name,
                                        const T& defaultValue) const
{
    if (!element.hasAttribute(name))
    {
        return defaultValue;
    }

    QString attrValue = element.attribute(name);

    if constexpr (std::is_same_v<T, QString>)
    {
        return attrValue;
    }
    if constexpr (std::is_same_v<T, std::string>)
    {
        return attrValue.toStdString();
    }
    if constexpr (std::is_same_v<T, bool>)
    {
        return attrValue == "true";
    }
    if constexpr (std::is_same_v<T, int>)
    {
        return attrValue.toInt();
    }
    if constexpr (std::is_same_v<T, uint32_t>)
    {
        return static_cast<uint32_t>(attrValue.toUInt());
    }
    if constexpr (std::is_same_v<T, qint64>)
    {
        return attrValue.toLongLong();
    }
    if constexpr (std::is_same_v<T, double>)
    {
        return attrValue.toDouble();
    }
    else
    {
        return static_cast<T>(attrValue.toLongLong());
    }
}

} // namespace MouseRecorder::Core::Serialization
