#pragma once

#include "core/IConfiguration.hpp"
#include "core/IEventRecorder.hpp"
#include "core/IEventPlayer.hpp"
#include "core/IEventStorage.hpp"
#include <memory>
#include <string>
#include <atomic>

namespace MouseRecorder::Application
{

/**
 * @brief Main application class that orchestrates all components
 *
 * This class serves as the main coordinator for the MouseRecorder application,
 * managing the lifecycle of recording sessions, playback sessions, and
 * providing a high-level API for the GUI layer.
 */
class MouseRecorderApp
{
  public:
    MouseRecorderApp();
    ~MouseRecorderApp();

    /**
     * @brief Initialize the application
     * @param configFile Configuration file path (optional)
     * @return true if initialization successful
     */
    bool initialize(const std::string& configFile = "");

    /**
     * @brief Initialize the application with options
     * @param configFile Configuration file path (optional)
     * @param headless Skip platform components initialization (for testing)
     * @return true if initialization successful
     */
    bool initialize(const std::string& configFile, bool headless);

    /**
     * @brief Initialize the application with log level override
     * @param configFile Configuration file path (optional)
     * @param headless Skip platform components initialization (for testing)
     * @param logLevelOverride Optional log level to override config
     * @return true if initialization successful
     */
    bool initialize(const std::string& configFile,
                    bool headless,
                    const std::string& logLevelOverride);

    /**
     * @brief Shutdown the application gracefully
     */
    void shutdown();

    /**
     * @brief Get configuration manager
     * @return reference to configuration
     */
    Core::IConfiguration& getConfiguration();

    /**
     * @brief Get event recorder
     * @return reference to recorder
     */
    Core::IEventRecorder& getEventRecorder();

    /**
     * @brief Get event player
     * @return reference to player
     */
    Core::IEventPlayer& getEventPlayer();

    /**
     * @brief Create storage handler for specified format
     * @param format Storage format
     * @return unique_ptr to storage handler
     */
    std::unique_ptr<Core::IEventStorage> createStorage(
        Core::StorageFormat format);

    /**
     * @brief Get application version
     * @return version string
     */
    static std::string getVersion();

    /**
     * @brief Get application name
     * @return application name
     */
    static std::string getApplicationName();

    /**
     * @brief Initialize logging system
     * @param config Configuration to use for logging setup
     * @return true if logging initialized successfully
     */
    static bool initializeLogging(Core::IConfiguration& config);

    /**
     * @brief Shutdown logging system
     */
    static void shutdownLogging();

    /**
     * @brief Get the last error message
     * @return error message
     */
    std::string getLastError() const;

  private:
    /**
     * @brief Setup platform-specific components
     * @return true if setup successful
     */
    bool setupPlatformComponents();

    /**
     * @brief Load configuration from file or create default
     * @param configFile Configuration file path
     * @return true if successful
     */
    bool loadConfiguration(const std::string& configFile);

    /**
     * @brief Set last error message
     * @param error Error message
     */
    void setLastError(const std::string& error);

  private:
    std::unique_ptr<Core::IConfiguration> m_configuration;
    std::unique_ptr<Core::IEventRecorder> m_eventRecorder;
    std::unique_ptr<Core::IEventPlayer> m_eventPlayer;

    bool m_initialized{false};
    std::atomic<bool> m_shuttingDown{false};
    std::string m_lastError;
    std::string m_configFile;
};

} // namespace MouseRecorder::Application
