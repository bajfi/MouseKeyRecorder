// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include "IEventSerializer.hpp"
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

namespace MouseRecorder::Core::Serialization
{

/**
 * @brief Qt-based XML implementation of event serialization
 *
 * This class uses Qt's QDomDocument and QDomElement for XML
 * serialization and deserialization, providing a Qt-native
 * alternative to third-party libraries like pugixml.
 */
class QtXmlEventSerializer : public IEventSerializer
{
  public:
    QtXmlEventSerializer();
    ~QtXmlEventSerializer() override = default;

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
     * @brief Convert Event to QDomElement
     * @param doc Parent document (for creating elements)
     * @param event Event to convert
     * @return QDomElement representation
     */
    QDomElement eventToXml(QDomDocument& doc, const Event& event) const;

    /**
     * @brief Convert QDomElement to Event
     * @param element QDomElement to convert
     * @return unique_ptr to Event or nullptr if conversion failed
     */
    std::unique_ptr<Event> xmlToEvent(const QDomElement& element) const;

    /**
     * @brief Convert StorageMetadata to QDomElement
     * @param doc Parent document (for creating elements)
     * @param metadata Metadata to convert
     * @return QDomElement representation
     */
    QDomElement metadataToXml(QDomDocument& doc,
                              const StorageMetadata& metadata) const;

    /**
     * @brief Convert QDomElement to StorageMetadata
     * @param element QDomElement to convert
     * @return StorageMetadata
     */
    StorageMetadata xmlToMetadata(const QDomElement& element) const;

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
     * @brief Convert Point to QDomElement
     * @param doc Parent document (for creating elements)
     * @param point Point to convert
     * @param elementName Name for the XML element
     * @return QDomElement with x and y attributes
     */
    QDomElement pointToXml(QDomDocument& doc,
                           const Point& point,
                           const QString& elementName) const;

    /**
     * @brief Convert QDomElement to Point
     * @param element XML element with x and y attributes
     * @return Point structure
     */
    Point xmlToPoint(const QDomElement& element) const;

    /**
     * @brief Set XML attribute safely (converts various types to string)
     */
    template <typename T>
    void setXmlAttribute(QDomElement& element,
                         const QString& name,
                         const T& value) const;

    /**
     * @brief Get XML attribute safely with default value
     */
    template <typename T>
    T getXmlAttribute(const QDomElement& element,
                      const QString& name,
                      const T& defaultValue = T{}) const;

    mutable std::string m_lastError;
    bool m_prettyFormat{true};
};

} // namespace MouseRecorder::Core::Serialization
