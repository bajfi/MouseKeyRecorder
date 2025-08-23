// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include "IEventSerializer.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace MouseRecorder::Core::Serialization
{

/**
 * @brief Qt-based JSON implementation of event serialization
 *
 * This class uses Qt's QJsonDocument, QJsonObject, and QJsonArray
 * for JSON serialization and deserialization, providing a Qt-native
 * alternative to third-party libraries like nlohmann::json.
 */
class QtJsonEventSerializer : public IEventSerializer
{
  public:
    QtJsonEventSerializer();
    ~QtJsonEventSerializer() override = default;

    // IEventSerializer interface
    std::string serializeEvents(
        const std::vector<std::unique_ptr<Event>>& events,
        const StorageMetadata& metadata,
        bool prettyFormat = true) const override;

    bool deserializeEvents(const std::string& data,
                           std::vector<std::unique_ptr<Event>>& events,
                           StorageMetadata& metadata) const override;

    std::string serializeMetadata(const StorageMetadata& metadata,
                                  bool prettyFormat = true) const override;

    bool deserializeMetadata(const std::string& data,
                             StorageMetadata& metadata) const override;

    bool validateFormat(const std::string& data) const override;

    SerializationFormat getSupportedFormat() const noexcept override;
    std::string getLibraryName() const noexcept override;
    std::string getLibraryVersion() const noexcept override;
    std::string getLastError() const override;
    bool supportsPrettyFormat() const noexcept override;

  protected:
    void setLastError(const std::string& error) const override;

  private:
    /**
     * @brief Convert Event to QJsonObject
     * @param event Event to convert
     * @return QJsonObject representation
     */
    QJsonObject eventToJson(const Event& event) const;

    /**
     * @brief Convert QJsonObject to Event
     * @param json QJsonObject to convert
     * @return unique_ptr to Event or nullptr if conversion failed
     */
    std::unique_ptr<Event> jsonToEvent(const QJsonObject& json) const;

    /**
     * @brief Convert StorageMetadata to QJsonObject
     * @param metadata Metadata to convert
     * @return QJsonObject representation
     */
    QJsonObject metadataToJson(const StorageMetadata& metadata) const;

    /**
     * @brief Convert QJsonObject to StorageMetadata
     * @param json QJsonObject to convert
     * @return StorageMetadata
     */
    StorageMetadata jsonToMetadata(const QJsonObject& json) const;

    /**
     * @brief Convert EventType enum to string
     * @param type EventType enum value
     * @return String representation
     */
    QString eventTypeToString(EventType type) const;

    /**
     * @brief Convert string to EventType enum
     * @param typeStr String representation
     * @return EventType enum value
     */
    EventType stringToEventType(const QString& typeStr) const;

    /**
     * @brief Convert Point to JSON object
     * @param point Point to convert
     * @return QJsonObject with x and y fields
     */
    QJsonObject pointToJson(const Point& point) const;

    /**
     * @brief Convert JSON object to Point
     * @param json JSON object with x and y fields
     * @return Point structure
     */
    Point jsonToPoint(const QJsonObject& json) const;

    mutable std::string m_lastError;
};

} // namespace MouseRecorder::Core::Serialization
