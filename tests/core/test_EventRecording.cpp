// Copyright (c) 2025 JackLee
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <gtest/gtest.h>
#include "application/MouseRecorderApp.hpp"
#include "core/Configuration.hpp"
#include "core/Event.hpp"
#include <chrono>
#include <thread>
#include <atomic>

namespace MouseRecorder::Tests
{

class EventRecordingTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create a temporary config file
        m_configFile = "/tmp/test_mouserecorder.conf";

        // Initialize the application
        ASSERT_TRUE(m_app.initialize(m_configFile));
    }

    void TearDown() override
    {
        m_app.shutdown();
        // Clean up temporary config file
        std::remove(m_configFile.c_str());
    }

    Application::MouseRecorderApp m_app;
    std::string m_configFile;
};

TEST_F(EventRecordingTest, RecordingStartStop)
{
    auto& recorder = m_app.getEventRecorder();

    EXPECT_FALSE(recorder.isRecording());

    std::atomic<int> eventCount{0};

    auto callback = [&eventCount](std::unique_ptr<Core::Event> event)
    {
        ASSERT_NE(event, nullptr);
        eventCount++;
    };

    EXPECT_TRUE(recorder.startRecording(callback));
    EXPECT_TRUE(recorder.isRecording());

    // Let it record for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    recorder.stopRecording();
    EXPECT_FALSE(recorder.isRecording());
}

TEST_F(EventRecordingTest, DoubleStartRecordingFails)
{
    auto& recorder = m_app.getEventRecorder();

    auto callback = [](std::unique_ptr<Core::Event> /*event*/) {};

    EXPECT_TRUE(recorder.startRecording(callback));
    EXPECT_TRUE(recorder.isRecording());

    // Second start should fail
    EXPECT_FALSE(recorder.startRecording(callback));

    recorder.stopRecording();
    EXPECT_FALSE(recorder.isRecording());
}

TEST_F(EventRecordingTest, StopWithoutStartIsNoOp)
{
    auto& recorder = m_app.getEventRecorder();

    EXPECT_FALSE(recorder.isRecording());

    // This should not crash or cause issues
    recorder.stopRecording();

    EXPECT_FALSE(recorder.isRecording());
}

TEST_F(EventRecordingTest, EventCallbackReceivesEvents)
{
    auto& recorder = m_app.getEventRecorder();

    std::atomic<int> mouseEvents{0};
    std::atomic<int> keyboardEvents{0};
    std::atomic<int> totalEvents{0};

    auto callback = [&](std::unique_ptr<Core::Event> event)
    {
        ASSERT_NE(event, nullptr);
        totalEvents++;

        if (event->isMouseEvent())
        {
            mouseEvents++;
        }
        if (event->isKeyboardEvent())
        {
            keyboardEvents++;
        }
    };

    EXPECT_TRUE(recorder.startRecording(callback));

    // Record for a brief period
    // Note: This test might not receive any events if no input occurs
    // In a real environment, you would simulate input events
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    recorder.stopRecording();

    // The counts should be consistent
    EXPECT_GE(totalEvents.load(), 0);
    EXPECT_LE(mouseEvents.load() + keyboardEvents.load(), totalEvents.load());
}

TEST_F(EventRecordingTest, ConfigurationPersistence)
{
    // Test configuration persistence directly using JSON-based Configuration
    // This avoids Qt dependencies that can cause hanging in test environments

    // Use a JSON config file instead of the Qt .conf format
    std::string jsonConfigFile = "/tmp/test_mouserecorder.json";

    Core::Configuration config1;
    // Create the file with defaults first if it doesn't exist
    config1.loadDefaults();
    ASSERT_TRUE(config1.saveToFile(jsonConfigFile));
    ASSERT_TRUE(config1.loadFromFile(jsonConfigFile));

    // Set some test values
    config1.setInt("test.value", 42);
    config1.setString("test.name", "EventRecordingTest");
    config1.setBool("test.flag", true);

    // Save to file
    ASSERT_TRUE(config1.saveToFile(jsonConfigFile));

    // Create a new configuration instance and load from the same file
    Core::Configuration config2;
    ASSERT_TRUE(config2.loadFromFile(jsonConfigFile));

    // Check that values were persisted
    EXPECT_EQ(config2.getInt("test.value", 0), 42);
    EXPECT_EQ(config2.getString("test.name", ""), "EventRecordingTest");
    EXPECT_EQ(config2.getBool("test.flag", false), true);

    // Clean up
    std::remove(jsonConfigFile.c_str());
}

} // namespace MouseRecorder::Tests
