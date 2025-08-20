// Copyright (c) 2025 JackLee
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "MouseRecorderApp.hpp"
#include "core/QtConfiguration.hpp"
#include "storage/EventStorageFactory.hpp"
#include "core/SpdlogConfig.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <filesystem>
#include <atomic>
#include <cstdlib>

#ifdef __linux__
#include "platform/linux/LinuxEventCapture.hpp"
#include "platform/linux/LinuxEventReplay.hpp"
#elif _WIN32
#include "platform/windows/WindowsEventCapture.hpp"
#include "platform/windows/WindowsEventReplay.hpp"
#endif

namespace MouseRecorder::Application
{

MouseRecorderApp::MouseRecorderApp()
{
    spdlog::debug("MouseRecorderApp: Constructor");
}

MouseRecorderApp::~MouseRecorderApp()
{
    // Only log if we haven't shut down yet (avoid logging after spdlog
    // shutdown)
    if (m_initialized && !m_shuttingDown.load())
    {
        spdlog::debug("MouseRecorderApp: Destructor");
    }

    shutdown();
}

bool MouseRecorderApp::initialize(const std::string& configFile)
{
    return initialize(configFile, false);
}

bool MouseRecorderApp::initialize(const std::string& configFile, bool headless)
{
    return initialize(configFile, headless, "");
}

bool MouseRecorderApp::initialize(const std::string& configFile,
                                  bool headless,
                                  const std::string& logLevelOverride)
{
    spdlog::info("MouseRecorderApp: Initializing application (headless: {})",
                 headless);

    if (m_initialized)
    {
        setLastError("Application is already initialized");
        return false;
    }

    m_configFile = configFile.empty() ? "mouserecorder.conf" : configFile;

    // Load configuration first
    if (!loadConfiguration(m_configFile))
    {
        return false;
    }

    // Override log level if provided
    if (!logLevelOverride.empty())
    {
        m_configuration->setString(Core::ConfigKeys::LOG_LEVEL,
                                   logLevelOverride);
        spdlog::info("MouseRecorderApp: Overriding log level to '{}'",
                     logLevelOverride);
    }

    // Initialize logging with configuration (including any overrides)
    if (!initializeLogging(*m_configuration))
    {
        setLastError("Failed to initialize logging system");
        return false;
    }

    // Setup platform-specific components
    if (!setupPlatformComponents())
    {
        return false;
    }

    m_initialized = true;
    spdlog::info("MouseRecorderApp: Application initialized successfully");

    return true;
}

void MouseRecorderApp::shutdown()
{
    // Use instance-based shutdown flag
    bool expected = false;
    if (!m_initialized || !m_shuttingDown.compare_exchange_weak(expected, true))
    {
        return;
    }

    try
    {
        spdlog::info("MouseRecorderApp: Shutting down application");

        // Stop any active recording/playback first (before destroying
        // components)
        try
        {
            if (m_eventRecorder && m_eventRecorder->isRecording())
            {
                spdlog::info("LinuxEventCapture: Stopping recording");
                m_eventRecorder->stopRecording();
            }
        }
        catch (const std::exception& e)
        {
            spdlog::warn("Error stopping recorder during shutdown: {}",
                         e.what());
        }

        try
        {
            if (m_eventPlayer &&
                m_eventPlayer->getState() != Core::PlaybackState::Stopped)
            {
                spdlog::info("LinuxEventReplay: Stopping playback");
                m_eventPlayer->stopPlayback();
            }
        }
        catch (const std::exception& e)
        {
            spdlog::warn("Error stopping player during shutdown: {}", e.what());
        }

        // Save configuration before destroying components
        if (m_configuration && !m_configFile.empty())
        {
            try
            {
                if (!m_configuration->saveToFile(m_configFile))
                {
                    spdlog::warn(
                        "MouseRecorderApp: Failed to save configuration: {}",
                        m_configuration->getLastError());
                }
            }
            catch (const std::exception& e)
            {
                spdlog::warn("Error saving configuration during shutdown: {}",
                             e.what());
            }
        }

        spdlog::info("MouseRecorderApp: Application shut down successfully");

        // Reset components after logging final messages
        m_eventPlayer.reset();
        m_eventRecorder.reset();
        m_configuration.reset();

        m_initialized = false;
    }
    catch (const std::exception& e)
    {
        spdlog::error("Exception during shutdown: {}", e.what());
    }

    // Only shutdown logging once globally, and only after all other cleanup
    shutdownLogging();
}

Core::IConfiguration& MouseRecorderApp::getConfiguration()
{
    if (!m_configuration)
    {
        throw std::runtime_error("Configuration not initialized");
    }
    return *m_configuration;
}

Core::IEventRecorder& MouseRecorderApp::getEventRecorder()
{
    if (!m_eventRecorder)
    {
        throw std::runtime_error("Event recorder not initialized");
    }
    return *m_eventRecorder;
}

Core::IEventPlayer& MouseRecorderApp::getEventPlayer()
{
    if (!m_eventPlayer)
    {
        throw std::runtime_error("Event player not initialized");
    }
    return *m_eventPlayer;
}

std::unique_ptr<Core::IEventStorage> MouseRecorderApp::createStorage(
    Core::StorageFormat format)
{
    return Storage::EventStorageFactory::createStorage(format);
}

std::string MouseRecorderApp::getVersion()
{
    return "0.0.1";
}

std::string MouseRecorderApp::getApplicationName()
{
    return "MouseRecorder";
}

bool MouseRecorderApp::initializeLogging(Core::IConfiguration& config)
{
    try
    {
        // Get logging configuration
        std::string logLevel =
            config.getString(Core::ConfigKeys::LOG_LEVEL, "info");
        bool logToFile = config.getBool(Core::ConfigKeys::LOG_TO_FILE, false);
        std::string logFilePath = config.getString(
            Core::ConfigKeys::LOG_FILE_PATH, "mouserecorder.log");

        // Convert log level string to spdlog level
        spdlog::level::level_enum level = spdlog::level::info;
        if (logLevel == "trace")
            level = spdlog::level::trace;
        else if (logLevel == "debug")
            level = spdlog::level::debug;
        else if (logLevel == "info")
            level = spdlog::level::info;
        else if (logLevel == "warn")
            level = spdlog::level::warn;
        else if (logLevel == "error")
            level = spdlog::level::err;
        else if (logLevel == "critical")
            level = spdlog::level::critical;
        else if (logLevel == "off")
            level = spdlog::level::off;

        // Create sinks
        std::vector<spdlog::sink_ptr> sinks;

        // Console sink (always enabled)
        auto console_sink =
            std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(level);
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%^%l%$] [%n] %v");
        sinks.push_back(console_sink);

        // File sink (if enabled)
        if (logToFile)
        {
            try
            {
                auto file_sink =
                    std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                        logFilePath, true);
                file_sink->set_level(level);
                file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%l] [%n] %v");
                sinks.push_back(file_sink);
            }
            catch (const std::exception& e)
            {
                // File sink creation failed, but continue with console only
                spdlog::warn("Failed to create file sink '{}': {}",
                             logFilePath,
                             e.what());
            }
        }

        // Create logger
        auto logger = std::make_shared<spdlog::logger>(
            "mouserecorder", sinks.begin(), sinks.end());
        logger->set_level(level);
        logger->flush_on(spdlog::level::warn);

        // Set as default logger
        spdlog::set_default_logger(logger);

        spdlog::info("Logging system initialized (level: {}, file: {})",
                     logLevel,
                     logToFile);
        return true;
    }
    catch (const std::exception& e)
    {
        // Fallback to basic console logging
        spdlog::set_level(spdlog::level::info);
        spdlog::error("Failed to initialize logging system: {}", e.what());
        return false;
    }
}

void MouseRecorderApp::shutdownLogging()
{
    spdlog::info("Skipping logging shutdown to avoid test issues");
    // Don't call spdlog::shutdown() in individual app instances
    // The test framework will handle global shutdown properly
    return;
}

std::string MouseRecorderApp::getLastError() const
{
    return m_lastError;
}

bool MouseRecorderApp::setupPlatformComponents()
{
    spdlog::debug("MouseRecorderApp: Setting up platform components");

    try
    {
#ifdef __linux__
        m_eventRecorder = std::make_unique<Platform::Linux::LinuxEventCapture>(
            *m_configuration);
        m_eventPlayer = std::make_unique<Platform::Linux::LinuxEventReplay>();
        spdlog::info("MouseRecorderApp: Linux platform components initialized");
#elif _WIN32
        m_eventRecorder =
            std::make_unique<Platform::Windows::WindowsEventCapture>(
                *m_configuration);
        m_eventPlayer =
            std::make_unique<Platform::Windows::WindowsEventReplay>();
        spdlog::info(
            "MouseRecorderApp: Windows platform components initialized");
#else
        setLastError("Unsupported platform");
        return false;
#endif

        // Configure components from settings
        if (m_configuration)
        {
            bool captureMouseEvents = m_configuration->getBool(
                Core::ConfigKeys::CAPTURE_MOUSE_EVENTS, true);
            bool captureKeyboardEvents = m_configuration->getBool(
                Core::ConfigKeys::CAPTURE_KEYBOARD_EVENTS, true);
            bool optimizeMouseMovements = m_configuration->getBool(
                Core::ConfigKeys::OPTIMIZE_MOUSE_MOVEMENTS, true);
            int mouseMovementThreshold = m_configuration->getInt(
                Core::ConfigKeys::MOUSE_MOVEMENT_THRESHOLD, 5);
            double playbackSpeed = m_configuration->getDouble(
                Core::ConfigKeys::DEFAULT_PLAYBACK_SPEED, 1.0);
            bool loopPlayback = m_configuration->getBool(
                Core::ConfigKeys::LOOP_PLAYBACK, false);

            m_eventRecorder->setCaptureMouseEvents(captureMouseEvents);
            m_eventRecorder->setCaptureKeyboardEvents(captureKeyboardEvents);
            m_eventRecorder->setOptimizeMouseMovements(optimizeMouseMovements);
            m_eventRecorder->setMouseMovementThreshold(mouseMovementThreshold);

            m_eventPlayer->setPlaybackSpeed(playbackSpeed);
            m_eventPlayer->setLoopPlayback(loopPlayback);

            spdlog::debug("MouseRecorderApp: Platform components configured "
                          "from settings");
        }

        return true;
    }
    catch (const std::exception& e)
    {
        setLastError("Platform setup error: " + std::string(e.what()));
        return false;
    }
}

bool MouseRecorderApp::loadConfiguration(const std::string& configFile)
{
    spdlog::debug("MouseRecorderApp: Loading configuration from {}",
                  configFile);

    try
    {
        m_configuration = std::make_unique<Core::QtConfiguration>();

        // Try to load from file
        if (std::filesystem::exists(configFile))
        {
            if (!m_configuration->loadFromFile(configFile))
            {
                spdlog::warn("MouseRecorderApp: Failed to load configuration "
                             "from {}: {}",
                             configFile,
                             m_configuration->getLastError());
                // Continue with defaults
            }
            else
            {
                spdlog::info("MouseRecorderApp: Configuration loaded from {}",
                             configFile);
            }
        }
        else
        {
            spdlog::info(
                "MouseRecorderApp: Configuration file {} not found, using "
                "defaults",
                configFile);
        }

        return true;
    }
    catch (const std::exception& e)
    {
        setLastError("Configuration load error: " + std::string(e.what()));
        return false;
    }
}

void MouseRecorderApp::setLastError(const std::string& error)
{
    m_lastError = error;
    spdlog::error("MouseRecorderApp: {}", error);
}

} // namespace MouseRecorder::Application
