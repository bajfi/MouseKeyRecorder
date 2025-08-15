#include "QtConfiguration.hpp"
#include "application/MouseRecorderApp.hpp"
#include <QStringList>
#include <QDir>
#include <QStandardPaths>
#include <spdlog/spdlog.h>
#include <limits>
#include <cmath>

namespace MouseRecorder::Core
{

QtConfiguration::QtConfiguration()
{
    spdlog::debug("QtConfiguration: Constructor");

    // Initialize QSettings with application info
    QString organization = "MouseRecorder";
    QString application = "MouseRecorder";

    m_settings = std::make_unique<QSettings>(organization, application);
    loadDefaults();
}

QtConfiguration::~QtConfiguration()
{
    // Safely destroy QSettings before Qt might be shut down

    spdlog::debug("QtConfiguration: Destructor");

    // Sync and reset settings before destruction
    if (m_settings)
    {
        m_settings->sync();
        m_settings.reset();
    }
}

bool QtConfiguration::loadFromFile(const std::string& filename)
{
    QMutexLocker locker(&m_mutex);
    spdlog::info("QtConfiguration: Loading from file {}", filename);

    try
    {
        // Create a temporary QSettings instance for this file
        QSettings fileSettings(toQString(filename), QSettings::IniFormat);

        if (fileSettings.status() != QSettings::NoError)
        {
            setLastError("Failed to open configuration file: " + filename);
            return false;
        }

        // Copy all settings from file to our main settings
        const QStringList keys = fileSettings.allKeys();
        for (const QString& key : keys)
        {
            QVariant value = fileSettings.value(key);
            m_settings->setValue(key, value);
        }

        m_settings->sync();

        if (m_settings->status() != QSettings::NoError)
        {
            setLastError("Failed to sync configuration settings");
            return false;
        }

        spdlog::info("QtConfiguration: Configuration loaded from {}", filename);
        return true;
    }
    catch (const std::exception& e)
    {
        setLastError("Configuration load error: " + std::string(e.what()));
        return false;
    }
}

bool QtConfiguration::saveToFile(const std::string& filename)
{
    QMutexLocker locker(&m_mutex);
    spdlog::info("QtConfiguration: Saving to file {}", filename);

    try
    {
        if (filename.empty())
        {
            // Save to default location
            m_settings->sync();
        }
        else
        {
            // Save to specific file
            QSettings fileSettings(toQString(filename), QSettings::IniFormat);

            const QStringList keys = m_settings->allKeys();
            for (const QString& key : keys)
            {
                QVariant value = m_settings->value(key);
                fileSettings.setValue(key, value);
            }

            fileSettings.sync();

            if (fileSettings.status() != QSettings::NoError)
            {
                setLastError(
                  "Failed to save configuration to file: " + filename
                );
                return false;
            }
        }

        if (m_settings->status() != QSettings::NoError)
        {
            setLastError("Failed to sync configuration settings");
            return false;
        }

        spdlog::info("QtConfiguration: Successfully saved configuration");
        return true;
    }
    catch (const std::exception& e)
    {
        setLastError("Configuration save error: " + std::string(e.what()));
        return false;
    }
}

void QtConfiguration::loadDefaults()
{
    QMutexLocker locker(&m_mutex);
    spdlog::debug("QtConfiguration: Loading default configuration");

    // Clear existing settings first if this is a reset
    // m_settings->clear();

    // Capture settings
    m_settings->setValue(toQString(ConfigKeys::CAPTURE_MOUSE_EVENTS), true);
    m_settings->setValue(toQString(ConfigKeys::CAPTURE_KEYBOARD_EVENTS), true);
    m_settings->setValue(toQString(ConfigKeys::OPTIMIZE_MOUSE_MOVEMENTS), true);
    m_settings->setValue(toQString(ConfigKeys::MOUSE_MOVEMENT_THRESHOLD), 5);

    // Playback settings
    m_settings->setValue(toQString(ConfigKeys::DEFAULT_PLAYBACK_SPEED), 1.0);
    m_settings->setValue(toQString(ConfigKeys::LOOP_PLAYBACK), false);

    // Logging settings
    m_settings->setValue(toQString(ConfigKeys::LOG_LEVEL), toQString("info"));
    m_settings->setValue(toQString(ConfigKeys::LOG_TO_FILE), false);
    m_settings->setValue(
      toQString(ConfigKeys::LOG_FILE_PATH), toQString("mouserecorder.log")
    );

    // UI settings
    m_settings->setValue(toQString(ConfigKeys::AUTO_MINIMIZE_ON_RECORD), true);

    m_settings->sync();
}

void QtConfiguration::setString(
  const std::string& key, const std::string& value
)
{
    QString oldValue;
    bool changed = false;

    // Critical section - hold mutex only for the actual setting operation
    {
        QMutexLocker locker(&m_mutex);
        oldValue = m_settings->value(toQString(key)).toString();
        m_settings->setValue(toQString(key), toQString(value));
        changed = (oldValue != toQString(value));
    }

    // Call callbacks outside of mutex to avoid deadlock
    if (changed)
    {
        notifyCallbacks(key, value);
    }
}

std::string QtConfiguration::getString(
  const std::string& key, const std::string& defaultValue
) const
{
    QMutexLocker locker(&m_mutex);
    return fromQString(
      m_settings->value(toQString(key), toQString(defaultValue)).toString()
    );
}

void QtConfiguration::setInt(const std::string& key, int value)
{
    int oldValue;
    bool changed = false;

    // Critical section - hold mutex only for the actual setting operation
    {
        QMutexLocker locker(&m_mutex);
        oldValue = m_settings->value(toQString(key)).toInt();
        m_settings->setValue(toQString(key), value);
        changed = (oldValue != value);
    }

    // Call callbacks outside of mutex to avoid deadlock
    if (changed)
    {
        notifyCallbacks(key, std::to_string(value));
    }
}

int QtConfiguration::getInt(const std::string& key, int defaultValue) const
{
    QMutexLocker locker(&m_mutex);
    return m_settings->value(toQString(key), defaultValue).toInt();
}

void QtConfiguration::setDouble(const std::string& key, double value)
{
    double oldValue;
    bool changed = false;

    // Critical section - hold mutex only for the actual setting operation
    {
        QMutexLocker locker(&m_mutex);
        oldValue = m_settings->value(toQString(key)).toDouble();
        m_settings->setValue(toQString(key), value);
        changed =
          (std::abs(oldValue - value) > std::numeric_limits<double>::epsilon());
    }

    // Call callbacks outside of mutex to avoid deadlock
    if (changed)
    {
        notifyCallbacks(key, std::to_string(value));
    }
}

double QtConfiguration::getDouble(
  const std::string& key, double defaultValue
) const
{
    QMutexLocker locker(&m_mutex);
    return m_settings->value(toQString(key), defaultValue).toDouble();
}

void QtConfiguration::setBool(const std::string& key, bool value)
{
    bool oldValue;
    bool changed = false;

    // Critical section - hold mutex only for the actual setting operation
    {
        QMutexLocker locker(&m_mutex);
        oldValue = m_settings->value(toQString(key)).toBool();
        m_settings->setValue(toQString(key), value);
        changed = (oldValue != value);
    }

    // Call callbacks outside of mutex to avoid deadlock
    if (changed)
    {
        notifyCallbacks(key, value ? "true" : "false");
    }
}

bool QtConfiguration::getBool(const std::string& key, bool defaultValue) const
{
    QMutexLocker locker(&m_mutex);
    return m_settings->value(toQString(key), defaultValue).toBool();
}

void QtConfiguration::setStringArray(
  const std::string& key, const std::vector<std::string>& value
)
{
    std::string valueStr;

    // Critical section - hold mutex only for the actual setting operation
    {
        QMutexLocker locker(&m_mutex);

        QStringList qList;
        for (const auto& str : value)
        {
            qList.append(toQString(str));
        }

        m_settings->setValue(toQString(key), qList);

        // For simplicity, always notify on array changes
        valueStr = "[";
        for (size_t i = 0; i < value.size(); ++i)
        {
            if (i > 0)
                valueStr += ",";
            valueStr += value[i];
        }
        valueStr += "]";
    }

    // Call callbacks outside of mutex to avoid deadlock
    notifyCallbacks(key, valueStr);
}

std::vector<std::string> QtConfiguration::getStringArray(
  const std::string& key, const std::vector<std::string>& defaultValue
) const
{
    QMutexLocker locker(&m_mutex);

    QStringList defaultQList;
    for (const auto& str : defaultValue)
    {
        defaultQList.append(toQString(str));
    }

    QStringList qList =
      m_settings->value(toQString(key), defaultQList).toStringList();

    std::vector<std::string> result;
    for (const QString& str : qList)
    {
        result.push_back(fromQString(str));
    }

    return result;
}

bool QtConfiguration::hasKey(const std::string& key) const
{
    QMutexLocker locker(&m_mutex);
    return m_settings->contains(toQString(key));
}

void QtConfiguration::removeKey(const std::string& key)
{
    bool keyExisted = false;

    // Critical section - hold mutex only for the actual removal operation
    {
        QMutexLocker locker(&m_mutex);
        keyExisted = m_settings->contains(toQString(key));
        if (keyExisted)
        {
            m_settings->remove(toQString(key));
        }
    }

    // Call callbacks outside of mutex to avoid deadlock
    if (keyExisted)
    {
        notifyCallbacks(key, "");
    }
}

std::vector<std::string> QtConfiguration::getAllKeys() const
{
    QMutexLocker locker(&m_mutex);
    QStringList qKeys = m_settings->allKeys();

    std::vector<std::string> keys;
    for (const QString& key : qKeys)
    {
        keys.push_back(fromQString(key));
    }

    return keys;
}

void QtConfiguration::clear()
{
    // Critical section - hold mutex only for the actual clear operation
    {
        QMutexLocker locker(&m_mutex);
        m_settings->clear();
    }

    // Call callbacks outside of mutex to avoid deadlock
    notifyCallbacks("*", "cleared");
}

size_t QtConfiguration::registerChangeCallback(ConfigChangeCallback callback)
{
    QMutexLocker locker(&m_mutex);
    size_t id = m_nextCallbackId++;
    m_callbacks[id] = callback;
    return id;
}

void QtConfiguration::unregisterChangeCallback(size_t callbackId)
{
    QMutexLocker locker(&m_mutex);
    m_callbacks.erase(callbackId);
}

std::string QtConfiguration::getLastError() const
{
    QMutexLocker locker(&m_errorMutex);
    return m_lastError;
}

QString QtConfiguration::toQString(const std::string& str) const
{
    return QString::fromStdString(str);
}

std::string QtConfiguration::fromQString(const QString& str) const
{
    return str.toStdString();
}

void QtConfiguration::notifyCallbacks(
  const std::string& key, const std::string& value
)
{
    // Don't hold the main mutex while calling callbacks to avoid deadlocks
    std::map<size_t, ConfigChangeCallback> callbacks;
    {
        QMutexLocker locker(&m_mutex);
        callbacks = m_callbacks;
    }

    for (const auto& [id, callback] : callbacks)
    {
        try
        {
            callback(key, value);
        }
        catch (const std::exception& e)
        {
            spdlog::warn(
              "QtConfiguration: Callback {} threw exception: {}", id, e.what()
            );
        }
    }
}

void QtConfiguration::setLastError(const std::string& error)
{
    QMutexLocker locker(&m_errorMutex);
    m_lastError = error;
    spdlog::error("QtConfiguration: {}", error);
}

} // namespace MouseRecorder::Core
