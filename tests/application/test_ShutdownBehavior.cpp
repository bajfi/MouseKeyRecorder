// Copyright (c) 2025 JackLee
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <gtest/gtest.h>
#include "application/MouseRecorderApp.hpp"
#include <chrono>
#include <thread>

namespace MouseRecorder::Tests
{

class ShutdownTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        m_configFile = "/tmp/test_shutdown_mouserecorder.conf";
    }

    void TearDown() override
    {
        // Clean up temporary config file
        std::remove(m_configFile.c_str());
    }

    std::string m_configFile;
};

TEST_F(ShutdownTest, CleanShutdownWithoutInitialization)
{
    Application::MouseRecorderApp app;

    // Should not crash even if shutdown is called without initialization
    EXPECT_NO_THROW(app.shutdown());
}

TEST_F(ShutdownTest, CleanShutdownAfterInitialization)
{
    Application::MouseRecorderApp app;

    ASSERT_TRUE(app.initialize(m_configFile));

    // Should shutdown cleanly
    EXPECT_NO_THROW(app.shutdown());
}

TEST_F(ShutdownTest, DoubleShutdownIsNoOp)
{
    Application::MouseRecorderApp app;

    ASSERT_TRUE(app.initialize(m_configFile));

    // First shutdown
    EXPECT_NO_THROW(app.shutdown());

    // Second shutdown should be a no-op and not crash
    EXPECT_NO_THROW(app.shutdown());
}

TEST_F(ShutdownTest, ShutdownWithActiveRecording)
{
    Application::MouseRecorderApp app;

    ASSERT_TRUE(app.initialize(m_configFile));

    auto& recorder = app.getEventRecorder();

    std::atomic<int> eventCount{0};
    auto callback = [&eventCount](std::unique_ptr<Core::Event> /*event*/)
    {
        eventCount++;
    };

    ASSERT_TRUE(recorder.startRecording(callback));
    ASSERT_TRUE(recorder.isRecording());

    // Shutdown should stop recording gracefully
    EXPECT_NO_THROW(app.shutdown());

    // After shutdown, we can't access the recorder anymore as components are
    // reset The test passes if shutdown doesn't crash
}

TEST_F(ShutdownTest, DestructorShutdown)
{
    // Test that destructor properly calls shutdown
    {
        Application::MouseRecorderApp app;
        ASSERT_TRUE(app.initialize(m_configFile));

        auto& recorder = app.getEventRecorder();

        std::atomic<int> eventCount{0};
        auto callback = [&eventCount](std::unique_ptr<Core::Event> /*event*/)
        {
            eventCount++;
        };

        ASSERT_TRUE(recorder.startRecording(callback));
        ASSERT_TRUE(recorder.isRecording());

        // App goes out of scope here - destructor should handle shutdown
        // We don't check the recorder state after this as it becomes invalid
    }

    // If we get here without crashing, the destructor worked correctly
    EXPECT_TRUE(true);
}

TEST_F(ShutdownTest, MultipleAppInstances)
{
    // Test that multiple app instances can be created and shut down
    for (int i = 0; i < 3; ++i)
    {
        Application::MouseRecorderApp app;
        std::string configFile = m_configFile + std::to_string(i);

        ASSERT_TRUE(app.initialize(configFile));
        EXPECT_NO_THROW(app.shutdown());

        // Clean up individual config file
        std::remove(configFile.c_str());
    }
}

} // namespace MouseRecorder::Tests
