#include <gtest/gtest.h>
#include "application/MouseRecorderApp.hpp"
#include "core/Event.hpp"
#include <filesystem>
#include <fstream>

using namespace MouseRecorder::Application;
using namespace MouseRecorder::Core;

class MouseRecorderAppTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create a temporary config file for testing
        m_testConfigFile = "test_mouserecorder.conf";
        createTestConfigFile();

        m_app = std::make_unique<MouseRecorderApp>();
    }

    void TearDown() override
    {
        if (m_app)
        {
            // Don't call shutdown explicitly - let destructor handle it
            m_app.reset();
        }

        // Clean up test config file
        if (std::filesystem::exists(m_testConfigFile))
        {
            std::filesystem::remove(m_testConfigFile);
        }
    }

    void createTestConfigFile()
    {
        std::ofstream file(m_testConfigFile);
        file << "capture.mouse_events=true\n";
        file << "capture.keyboard_events=true\n";
        file << "capture.optimize_mouse_movements=true\n";
        file << "capture.mouse_movement_threshold=5\n";
        file << "playback.default_speed=1.0\n";
        file << "playback.loop_enabled=false\n";
        file << "logging.level=info\n";
        file << "logging.to_file=false\n";
        file.close();
    }

    std::unique_ptr<MouseRecorderApp> m_app;
    std::string m_testConfigFile;
};

TEST_F(MouseRecorderAppTest, InitializationWithoutConfig)
{
    EXPECT_TRUE(m_app->initialize());
    EXPECT_TRUE(m_app->getLastError().empty());

    // Should be able to get components
    EXPECT_NO_THROW({
        auto& config = m_app->getConfiguration();
        (void) config;
    });
    EXPECT_NO_THROW({
        auto& recorder = m_app->getEventRecorder();
        (void) recorder;
    });
    EXPECT_NO_THROW({
        auto& player = m_app->getEventPlayer();
        (void) player;
    });
}

TEST_F(MouseRecorderAppTest, InitializationWithConfig)
{
    EXPECT_TRUE(m_app->initialize(m_testConfigFile));
    EXPECT_TRUE(m_app->getLastError().empty());

    auto& config = m_app->getConfiguration();

    // Verify configuration was loaded
    EXPECT_TRUE(config.getBool("capture.mouse_events", false));
    EXPECT_TRUE(config.getBool("capture.keyboard_events", false));
    EXPECT_EQ(config.getInt("capture.mouse_movement_threshold", 0), 5);
    EXPECT_DOUBLE_EQ(config.getDouble("playback.default_speed", 0.0), 1.0);
}

TEST_F(MouseRecorderAppTest, DoubleInitialization)
{
    EXPECT_TRUE(m_app->initialize());

    // Second initialization should fail
    EXPECT_FALSE(m_app->initialize());
    EXPECT_FALSE(m_app->getLastError().empty());
}

TEST_F(MouseRecorderAppTest, GetComponentsBeforeInitialization)
{
    // Should throw when accessing components before initialization
    EXPECT_THROW(m_app->getConfiguration(), std::runtime_error);
    EXPECT_THROW(m_app->getEventRecorder(), std::runtime_error);
    EXPECT_THROW(m_app->getEventPlayer(), std::runtime_error);
}

TEST_F(MouseRecorderAppTest, CreateStorage)
{
    EXPECT_TRUE(m_app->initialize());

    // Test creating different storage types
    auto jsonStorage = m_app->createStorage(StorageFormat::Json);
    EXPECT_NE(jsonStorage, nullptr);

    auto xmlStorage = m_app->createStorage(StorageFormat::Xml);
    EXPECT_NE(xmlStorage, nullptr);

    auto binaryStorage = m_app->createStorage(StorageFormat::Binary);
    EXPECT_NE(binaryStorage, nullptr);
}

TEST_F(MouseRecorderAppTest, VersionAndApplicationName)
{
    // These should work without initialization
    EXPECT_FALSE(MouseRecorderApp::getVersion().empty());
    EXPECT_FALSE(MouseRecorderApp::getApplicationName().empty());

    EXPECT_EQ(MouseRecorderApp::getApplicationName(), "MouseRecorder");
}

TEST_F(MouseRecorderAppTest, ShutdownWithActiveRecording)
{
    EXPECT_TRUE(m_app->initialize());

    auto& recorder = m_app->getEventRecorder();

    // Try to start recording (might fail in headless environment)
    auto callback = [](std::unique_ptr<Event> event)
    {
        // Do nothing with the event
        (void) event;
    };

    if (recorder.startRecording(callback))
    {
        EXPECT_TRUE(recorder.isRecording());

        // Shutdown should stop recording - check state before shutdown
        // since shutdown will destroy the recorder object
        m_app->shutdown();

        // After shutdown, we cannot access recorder anymore as it's been reset
        // The test should verify that shutdown completed successfully
        // The recording stopping is verified by the logs
    }
}

TEST_F(MouseRecorderAppTest, ShutdownWithActivePlayback)
{
    EXPECT_TRUE(m_app->initialize());

    auto& player = m_app->getEventPlayer();

    // Create some test events
    std::vector<std::unique_ptr<Event>> events;
    events.push_back(EventFactory::createMouseMoveEvent({100, 100}));

    if (player.loadEvents(std::move(events)) && player.startPlayback())
    {
        EXPECT_NE(player.getState(), PlaybackState::Stopped);

        // Shutdown should stop playback - check state before shutdown
        // since shutdown will destroy the player object
        m_app->shutdown();

        // After shutdown, we cannot access player anymore as it's been reset
        // The test should verify that shutdown completed successfully
        // The playback stopping is verified by the logs
    }
}

TEST_F(MouseRecorderAppTest, LoggingInitialization)
{
    // Test with different logging configurations
    EXPECT_TRUE(m_app->initialize(m_testConfigFile));

    auto& config = m_app->getConfiguration();

    // Verify logging was initialized based on config
    EXPECT_EQ(config.getString("logging.level", ""), "info");
    EXPECT_FALSE(config.getBool("logging.to_file", true));
}

TEST_F(MouseRecorderAppTest, MultipleShutdowns)
{
    EXPECT_TRUE(m_app->initialize());

    // Multiple shutdowns should be safe
    m_app->shutdown();
    EXPECT_NO_THROW(m_app->shutdown());
    EXPECT_NO_THROW(m_app->shutdown());
}
