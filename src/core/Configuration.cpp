// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "Configuration.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include "core/SpdlogConfig.hpp"

using json = nlohmann::json;

namespace MouseRecorder::Core
{

Configuration::Configuration()
{
    spdlog::debug("Configuration: Constructor");
    loadDefaults();
}

bool Configuration::loadFromFile(const std::string& filename)
{
    spdlog::info("Configuration: Loading from file {}", filename);

    try
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            setLastError("Failed to open configuration file: " + filename);
            return false;
        }

        json configJson;
        file >> configJson;

        std::lock_guard<std::mutex> lock(m_mutex);
        m_values.clear();

        for (auto& [key, value] : configJson.items())
        {
            if (value.is_string())
            {
                m_values[key] = value.get<std::string>();
            }
            else if (value.is_number_integer())
            {
                m_values[key] = value.get<int>();
            }
            else if (value.is_number_float())
            {
                m_values[key] = value.get<double>();
            }
            else if (value.is_boolean())
            {
                m_values[key] = value.get<bool>();
            }
            else if (value.is_array() && !value.empty() && value[0].is_string())
            {
                m_values[key] = value.get<std::vector<std::string>>();
            }
        }

        spdlog::info("Configuration: Successfully loaded {} settings",
                     m_values.size());
        return true;
    }
    catch (const std::exception& e)
    {
        setLastError("Configuration load error: " + std::string(e.what()));
        return false;
    }
}

bool Configuration::saveToFile(const std::string& filename)
{
    spdlog::info("Configuration: Saving to file {}", filename);

    try
    {
        json configJson;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (const auto& [key, value] : m_values)
            {
                std::visit(
                    [&](const auto& v)
                    {
                        configJson[key] = v;
                    },
                    value);
            }
        }

        std::ofstream file(filename);
        if (!file.is_open())
        {
            setLastError("Failed to open configuration file for writing: " +
                         filename);
            return false;
        }

        file << configJson.dump(2);

        if (file.fail())
        {
            setLastError("Failed to write configuration file");
            return false;
        }

        spdlog::info("Configuration: Successfully saved configuration");
        return true;
    }
    catch (const std::exception& e)
    {
        setLastError("Configuration save error: " + std::string(e.what()));
        return false;
    }
}

void Configuration::loadDefaults()
{
    spdlog::debug("Configuration: Loading default values");

    std::lock_guard<std::mutex> lock(m_mutex);

    // Recording settings
    m_values[ConfigKeys::CAPTURE_MOUSE_EVENTS] = true;
    m_values[ConfigKeys::CAPTURE_KEYBOARD_EVENTS] = true;
    m_values[ConfigKeys::OPTIMIZE_MOUSE_MOVEMENTS] = true;
    m_values[ConfigKeys::MOUSE_MOVEMENT_THRESHOLD] = 5;
    m_values[ConfigKeys::MOUSE_OPTIMIZATION_TIME_THRESHOLD] = 16; // ~60fps
    m_values[ConfigKeys::MOUSE_OPTIMIZATION_DOUGLAS_PEUCKER_EPSILON] = 2.0;
    m_values[ConfigKeys::MOUSE_OPTIMIZATION_PRESERVE_CLICKS] = true;
    m_values[ConfigKeys::MOUSE_OPTIMIZATION_PRESERVE_FIRST_LAST] = true;
    m_values[ConfigKeys::MOUSE_OPTIMIZATION_STRATEGY] = std::string("combined");
    m_values[ConfigKeys::DEFAULT_STORAGE_FORMAT] = std::string("json");
    m_values[ConfigKeys::FILTER_STOP_RECORDING_SHORTCUT] = true;

    // Playback settings
    m_values[ConfigKeys::DEFAULT_PLAYBACK_SPEED] = 1.0;
    m_values[ConfigKeys::LOOP_PLAYBACK] = false;
    m_values[ConfigKeys::SHOW_PLAYBACK_CURSOR] = true;

    // UI settings
    m_values[ConfigKeys::WINDOW_WIDTH] = 800;
    m_values[ConfigKeys::WINDOW_HEIGHT] = 600;
    m_values[ConfigKeys::WINDOW_X] = 100;
    m_values[ConfigKeys::WINDOW_Y] = 100;
    m_values[ConfigKeys::WINDOW_MAXIMIZED] = false;
    m_values[ConfigKeys::THEME] = std::string("system");
    m_values[ConfigKeys::LANGUAGE] = std::string("en");
    m_values[ConfigKeys::AUTO_MINIMIZE_ON_RECORD] = true;

    // Keyboard shortcuts
    m_values[ConfigKeys::SHORTCUT_START_RECORDING] = std::string("Ctrl+R");
    m_values[ConfigKeys::SHORTCUT_STOP_RECORDING] = std::string("Ctrl+Shift+R");
    m_values[ConfigKeys::SHORTCUT_START_PLAYBACK] = std::string("Ctrl+P");
    m_values[ConfigKeys::SHORTCUT_STOP_PLAYBACK] = std::string("Ctrl+Shift+P");

    // File paths
    m_values[ConfigKeys::LAST_SAVE_DIRECTORY] = std::string("");
    m_values[ConfigKeys::LAST_OPEN_DIRECTORY] = std::string("");
    m_values[ConfigKeys::RECENT_FILES] = std::vector<std::string>{};

    // System settings
    m_values[ConfigKeys::LOG_LEVEL] = std::string("info");
    m_values[ConfigKeys::LOG_TO_FILE] = false;
    m_values[ConfigKeys::LOG_FILE_PATH] = std::string("mouserecorder.log");

    spdlog::debug("Configuration: Default values loaded");
}

void Configuration::setString(const std::string& key, const std::string& value)
{
    setValue(key, value);
}

std::string Configuration::getString(const std::string& key,
                                     const std::string& defaultValue) const
{
    return getValue(key, defaultValue);
}

void Configuration::setInt(const std::string& key, int value)
{
    setValue(key, value);
}

int Configuration::getInt(const std::string& key, int defaultValue) const
{
    return getValue(key, defaultValue);
}

void Configuration::setDouble(const std::string& key, double value)
{
    setValue(key, value);
}

double Configuration::getDouble(const std::string& key,
                                double defaultValue) const
{
    return getValue(key, defaultValue);
}

void Configuration::setBool(const std::string& key, bool value)
{
    setValue(key, value);
}

bool Configuration::getBool(const std::string& key, bool defaultValue) const
{
    return getValue(key, defaultValue);
}

void Configuration::setStringArray(const std::string& key,
                                   const std::vector<std::string>& value)
{
    setValue(key, value);
}

std::vector<std::string> Configuration::getStringArray(
    const std::string& key, const std::vector<std::string>& defaultValue) const
{
    return getValue(key, defaultValue);
}

bool Configuration::hasKey(const std::string& key) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_values.find(key) != m_values.end();
}

void Configuration::removeKey(const std::string& key)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_values.find(key);
    if (it != m_values.end())
    {
        m_values.erase(it);
        spdlog::debug("Configuration: Removed key '{}'", key);
    }
}

std::vector<std::string> Configuration::getAllKeys() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> keys;
    keys.reserve(m_values.size());

    for (const auto& [key, value] : m_values)
    {
        keys.push_back(key);
    }

    return keys;
}

void Configuration::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_values.clear();
    spdlog::debug("Configuration: Cleared all values");
}

size_t Configuration::registerChangeCallback(ConfigChangeCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t id = m_nextCallbackId++;
    m_callbacks[id] = std::move(callback);
    spdlog::debug("Configuration: Registered change callback with ID {}", id);
    return id;
}

void Configuration::unregisterChangeCallback(size_t callbackId)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (auto it = m_callbacks.find(callbackId); it != m_callbacks.end())
    {
        m_callbacks.erase(it);
        spdlog::debug("Configuration: Unregistered change callback with ID {}",
                      callbackId);
    }
}

std::string Configuration::getLastError() const
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    return m_lastError;
}

template <typename T>
void Configuration::setValue(const std::string& key, const T& value)
{
    std::string oldValueStr;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Get old value for comparison
        if (auto it = m_values.find(key); it != m_values.end())
        {
            oldValueStr = valueToString(it->second);
        }

        m_values[key] = value;
    }

    std::string newValueStr = valueToString(ConfigValue{value});

    // Only notify if value actually changed
    if (oldValueStr != newValueStr)
    {
        notifyCallbacks(key, newValueStr);
        spdlog::debug("Configuration: Set '{}' = '{}'", key, newValueStr);
    }
}

template <typename T>
T Configuration::getValue(const std::string& key, const T& defaultValue) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (auto it = m_values.find(key); it != m_values.end())
    {
        try
        {
            return std::get<T>(it->second);
        }
        catch (const std::bad_variant_access&)
        {
            spdlog::warn(
                "Configuration: Type mismatch for key '{}', returning default",
                key);
        }
    }

    return defaultValue;
}

void Configuration::notifyCallbacks(const std::string& key,
                                    const std::string& value)
{
    std::vector<ConfigChangeCallback> callbacks;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        callbacks.reserve(m_callbacks.size());
        for (const auto& [id, callback] : m_callbacks)
        {
            callbacks.push_back(callback);
        }
    }

    // Call callbacks outside of lock to avoid deadlock
    for (const auto& callback : callbacks)
    {
        try
        {
            callback(key, value);
        }
        catch (const std::exception& e)
        {
            spdlog::error("Configuration: Callback exception for key '{}': {}",
                          key,
                          e.what());
        }
    }
}

std::string Configuration::valueToString(const ConfigValue& value) const
{
    return std::visit(
        [](const auto& v) -> std::string
        {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>)
            {
                return v;
            }
            else if constexpr (std::is_same_v<T, int>)
            {
                return std::to_string(v);
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                return std::to_string(v);
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                return v ? "true" : "false";
            }
            else if constexpr (std::is_same_v<T, std::vector<std::string>>)
            {
                std::string result = "[";
                for (size_t i = 0; i < v.size(); ++i)
                {
                    if (i > 0)
                        result += ", ";
                    result += "\"" + v[i] + "\"";
                }
                result += "]";
                return result;
            }
            return "";
        },
        value);
}

void Configuration::setLastError(const std::string& error)
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    m_lastError = error;
    spdlog::error("Configuration: {}", error);
}

} // namespace MouseRecorder::Core
