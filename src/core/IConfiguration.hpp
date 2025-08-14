#pragma once

#include <string>
#include <vector>
#include <functional>
#include <optional>

namespace MouseRecorder::Core
{

/**
 * @brief Configuration change callback type
 */
using ConfigChangeCallback =
  std::function<void(const std::string& key, const std::string& value)>;

/**
 * @brief Interface for application configuration management
 *
 * This interface provides a unified way to manage application settings
 * with type-safe getters/setters and change notifications.
 */
class IConfiguration
{
  public:
    virtual ~IConfiguration() = default;

    /**
     * @brief Load configuration from file
     * @param filename Configuration file path
     * @return true if loaded successfully
     */
    virtual bool loadFromFile(const std::string& filename) = 0;

    /**
     * @brief Save configuration to file
     * @param filename Configuration file path
     * @return true if saved successfully
     */
    virtual bool saveToFile(const std::string& filename) = 0;

    /**
     * @brief Load default configuration
     */
    virtual void loadDefaults() = 0;

    // String values
    virtual void setString(
      const std::string& key, const std::string& value
    ) = 0;
    virtual std::string getString(
      const std::string& key, const std::string& defaultValue = ""
    ) const = 0;

    // Integer values
    virtual void setInt(const std::string& key, int value) = 0;
    virtual int getInt(const std::string& key, int defaultValue = 0) const = 0;

    // Double values
    virtual void setDouble(const std::string& key, double value) = 0;
    virtual double getDouble(
      const std::string& key, double defaultValue = 0.0
    ) const = 0;

    // Boolean values
    virtual void setBool(const std::string& key, bool value) = 0;
    virtual bool getBool(
      const std::string& key, bool defaultValue = false
    ) const = 0;

    // Array values
    virtual void setStringArray(
      const std::string& key, const std::vector<std::string>& value
    ) = 0;
    virtual std::vector<std::string> getStringArray(
      const std::string& key, const std::vector<std::string>& defaultValue = {}
    ) const = 0;

    /**
     * @brief Check if a key exists
     * @param key Configuration key
     * @return true if key exists
     */
    virtual bool hasKey(const std::string& key) const = 0;

    /**
     * @brief Remove a configuration key
     * @param key Configuration key to remove
     */
    virtual void removeKey(const std::string& key) = 0;

    /**
     * @brief Get all configuration keys
     * @return vector of all keys
     */
    virtual std::vector<std::string> getAllKeys() const = 0;

    /**
     * @brief Clear all configuration
     */
    virtual void clear() = 0;

    /**
     * @brief Register callback for configuration changes
     * @param callback Function to call when configuration changes
     * @return callback ID for unregistering
     */
    virtual size_t registerChangeCallback(ConfigChangeCallback callback) = 0;

    /**
     * @brief Unregister configuration change callback
     * @param callbackId ID returned from registerChangeCallback
     */
    virtual void unregisterChangeCallback(size_t callbackId) = 0;

    /**
     * @brief Get the last error message if any operation failed
     * @return error message or empty string if no error
     */
    virtual std::string getLastError() const = 0;
};

/**
 * @brief Standard configuration keys used by the application
 */
namespace ConfigKeys
{
// Recording settings
constexpr const char* CAPTURE_MOUSE_EVENTS = "recording.capture_mouse_events";
constexpr const char* CAPTURE_KEYBOARD_EVENTS =
  "recording.capture_keyboard_events";
constexpr const char* OPTIMIZE_MOUSE_MOVEMENTS =
  "recording.optimize_mouse_movements";
constexpr const char* MOUSE_MOVEMENT_THRESHOLD =
  "recording.mouse_movement_threshold";
constexpr const char* DEFAULT_STORAGE_FORMAT =
  "recording.default_storage_format";

// Playback settings
constexpr const char* DEFAULT_PLAYBACK_SPEED = "playback.default_speed";
constexpr const char* LOOP_PLAYBACK = "playback.loop_enabled";
constexpr const char* SHOW_PLAYBACK_CURSOR = "playback.show_cursor";

// UI settings
constexpr const char* WINDOW_WIDTH = "ui.window_width";
constexpr const char* WINDOW_HEIGHT = "ui.window_height";
constexpr const char* WINDOW_X = "ui.window_x";
constexpr const char* WINDOW_Y = "ui.window_y";
constexpr const char* WINDOW_MAXIMIZED = "ui.window_maximized";
constexpr const char* THEME = "ui.theme";
constexpr const char* LANGUAGE = "ui.language";
constexpr const char* AUTO_MINIMIZE_ON_RECORD = "ui.auto_minimize_on_record";

// Keyboard shortcuts
constexpr const char* SHORTCUT_START_RECORDING = "shortcuts.start_recording";
constexpr const char* SHORTCUT_STOP_RECORDING = "shortcuts.stop_recording";
constexpr const char* SHORTCUT_START_PLAYBACK = "shortcuts.start_playback";
constexpr const char* SHORTCUT_STOP_PLAYBACK = "shortcuts.stop_playback";
constexpr const char* SHORTCUT_PAUSE_PLAYBACK = "shortcuts.pause_playback";

// File paths
constexpr const char* LAST_SAVE_DIRECTORY = "files.last_save_directory";
constexpr const char* LAST_OPEN_DIRECTORY = "files.last_open_directory";
constexpr const char* RECENT_FILES = "files.recent_files";

// System settings
constexpr const char* LOG_LEVEL = "system.log_level";
constexpr const char* LOG_TO_FILE = "system.log_to_file";
constexpr const char* LOG_FILE_PATH = "system.log_file_path";
} // namespace ConfigKeys

} // namespace MouseRecorder::Core
