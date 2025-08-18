#pragma once

#include "IConfiguration.hpp"
#include <QSettings>
#include <QMutex>
#include <QMutexLocker>
#include <memory>
#include <map>

namespace MouseRecorder::Core
{

/**
 * @brief QSettings-based configuration implementation
 *
 * This class provides a thread-safe implementation of configuration storage
 * using Qt's QSettings for cross-platform persistence.
 */
class QtConfiguration : public IConfiguration
{
  public:
    QtConfiguration();
    ~QtConfiguration() override;

    // IConfiguration interface
    bool loadFromFile(const std::string& filename) override;
    bool saveToFile(const std::string& filename) override;
    void loadDefaults() override;

    // String values
    void setString(const std::string& key, const std::string& value) override;
    std::string getString(const std::string& key,
                          const std::string& defaultValue = "") const override;

    // Integer values
    void setInt(const std::string& key, int value) override;
    int getInt(const std::string& key, int defaultValue = 0) const override;

    // Double values
    void setDouble(const std::string& key, double value) override;
    double getDouble(const std::string& key,
                     double defaultValue = 0.0) const override;

    // Boolean values
    void setBool(const std::string& key, bool value) override;
    bool getBool(const std::string& key,
                 bool defaultValue = false) const override;

    // Array values
    void setStringArray(const std::string& key,
                        const std::vector<std::string>& value) override;
    std::vector<std::string> getStringArray(
        const std::string& key,
        const std::vector<std::string>& defaultValue = {}) const override;

    // Key management
    bool hasKey(const std::string& key) const override;
    void removeKey(const std::string& key) override;
    std::vector<std::string> getAllKeys() const override;
    void clear() override;

    // Change notifications
    size_t registerChangeCallback(ConfigChangeCallback callback) override;
    void unregisterChangeCallback(size_t callbackId) override;

    std::string getLastError() const override;

  private:
    /**
     * @brief Convert std::string to QString
     */
    QString toQString(const std::string& str) const;

    /**
     * @brief Convert QString to std::string
     */
    std::string fromQString(const QString& str) const;

    /**
     * @brief Notify all registered callbacks about a change
     * @param key Changed key
     * @param value New value as string
     */
    void notifyCallbacks(const std::string& key, const std::string& value);

    /**
     * @brief Set last error message in thread-safe manner
     * @param error Error message
     */
    void setLastError(const std::string& error);

  private:
    mutable QMutex m_mutex;
    std::unique_ptr<QSettings> m_settings;

    // Callback management
    std::map<size_t, ConfigChangeCallback> m_callbacks;
    size_t m_nextCallbackId{1};

    // Error handling
    mutable QMutex m_errorMutex;
    std::string m_lastError;
};

} // namespace MouseRecorder::Core
