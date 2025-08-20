// Copyright (c) 2025 JackLee
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include "IConfiguration.hpp"
#include <map>
#include <mutex>
#include <variant>

namespace MouseRecorder::Core
{

/**
 * @brief Concrete implementation of configuration management
 *
 * This class provides a thread-safe implementation of configuration storage
 * using JSON files for persistence and in-memory storage for fast access.
 */
class Configuration : public IConfiguration
{
  public:
    Configuration();
    ~Configuration() override = default;

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
    using ConfigValue =
        std::variant<std::string, int, double, bool, std::vector<std::string>>;

    /**
     * @brief Set a configuration value with type safety
     * @param key Configuration key
     * @param value Value to set
     */
    template <typename T> void setValue(const std::string& key, const T& value);

    /**
     * @brief Get a configuration value with type safety
     * @param key Configuration key
     * @param defaultValue Default value if key doesn't exist
     * @return value or default
     */
    template <typename T>
    T getValue(const std::string& key, const T& defaultValue) const;

    /**
     * @brief Notify all registered callbacks about a change
     * @param key Changed key
     * @param value New value as string
     */
    void notifyCallbacks(const std::string& key, const std::string& value);

    /**
     * @brief Convert ConfigValue to string representation
     * @param value Value to convert
     * @return string representation
     */
    std::string valueToString(const ConfigValue& value) const;

    /**
     * @brief Set last error message in thread-safe manner
     * @param error Error message
     */
    void setLastError(const std::string& error);

  private:
    mutable std::mutex m_mutex;
    std::map<std::string, ConfigValue> m_values;

    // Callback management
    std::map<size_t, ConfigChangeCallback> m_callbacks;
    size_t m_nextCallbackId{1};

    // Error handling
    mutable std::mutex m_errorMutex;
    std::string m_lastError;
};

} // namespace MouseRecorder::Core
