// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <gtest/gtest.h>
#include "platform/windows/WindowsEventCapture.hpp"
#include "core/QtConfiguration.hpp"
#include <memory>

using namespace MouseRecorder::Platform::Windows;
using namespace MouseRecorder::Core;

class WindowsEventCaptureTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create a test configuration
        m_config = std::make_unique<QtConfiguration>();

        // Set default configuration values
        m_config->setBool(ConfigKeys::CAPTURE_MOUSE_EVENTS, true);
        m_config->setBool(ConfigKeys::CAPTURE_KEYBOARD_EVENTS, true);
        m_config->setBool(ConfigKeys::OPTIMIZE_MOUSE_MOVEMENTS, false);
        m_config->setInt(ConfigKeys::MOUSE_MOVEMENT_THRESHOLD, 5);

        // Create the event capture instance
        m_eventCapture = std::make_unique<WindowsEventCapture>(*m_config);
    }

    void TearDown() override
    {
        if (m_eventCapture && m_eventCapture->isRecording())
        {
            m_eventCapture->stopRecording();
        }
        m_eventCapture.reset();
        m_config.reset();
    }

    std::unique_ptr<QtConfiguration> m_config;
    std::unique_ptr<WindowsEventCapture> m_eventCapture;
};

TEST_F(WindowsEventCaptureTest, ConstructorInitializesCorrectly)
{
    EXPECT_FALSE(m_eventCapture->isRecording());
    EXPECT_TRUE(m_eventCapture->getLastError().empty());
}

TEST_F(WindowsEventCaptureTest, ConfigurationSettings)
{
    // Test mouse event capture setting
    m_eventCapture->setCaptureMouseEvents(false);
    m_eventCapture->setCaptureMouseEvents(true);

    // Test keyboard event capture setting
    m_eventCapture->setCaptureKeyboardEvents(false);
    m_eventCapture->setCaptureKeyboardEvents(true);

    // Test mouse movement optimization
    m_eventCapture->setOptimizeMouseMovements(true);
    m_eventCapture->setOptimizeMouseMovements(false);

    // Test mouse movement threshold
    m_eventCapture->setMouseMovementThreshold(10);
    m_eventCapture->setMouseMovementThreshold(1);

    // These should not throw exceptions
    SUCCEED();
}

#ifdef _WIN32
TEST_F(WindowsEventCaptureTest, StartRecordingWithCallback)
{
    bool callbackInvoked = false;
    std::unique_ptr<Event> receivedEvent;

    auto callback = [&](std::unique_ptr<Event> event)
    {
        callbackInvoked = true;
        receivedEvent = std::move(event);
    };

    // On Windows, this should succeed
    bool result = m_eventCapture->startRecording(callback);
    EXPECT_TRUE(result) << "Recording should start successfully on Windows";

    if (result)
    {
        EXPECT_TRUE(m_eventCapture->isRecording());

        // Stop recording
        m_eventCapture->stopRecording();
        EXPECT_FALSE(m_eventCapture->isRecording());
    }
}

TEST_F(WindowsEventCaptureTest, StopRecordingWhenNotRecording)
{
    // Should not crash when stopping recording that hasn't started
    EXPECT_FALSE(m_eventCapture->isRecording());
    m_eventCapture->stopRecording();
    EXPECT_FALSE(m_eventCapture->isRecording());
}

TEST_F(WindowsEventCaptureTest, MultipleStartStopCycles)
{
    auto callback = [](std::unique_ptr<Event> event)
    {
        (void) event; // Suppress unused parameter warning
        // Empty callback for testing
    };

    for (int i = 0; i < 3; ++i)
    {
        bool result = m_eventCapture->startRecording(callback);
        EXPECT_TRUE(result)
            << "Recording should start successfully on iteration " << i;

        if (result)
        {
            EXPECT_TRUE(m_eventCapture->isRecording());
            m_eventCapture->stopRecording();
            EXPECT_FALSE(m_eventCapture->isRecording());
        }
    }
}
#else
TEST_F(WindowsEventCaptureTest, StartRecordingFailsOnNonWindows)
{
    auto callback = [](std::unique_ptr<Event> event)
    {
        (void) event; // Suppress unused parameter warning
        // Empty callback
    };

    // On non-Windows platforms, this should fail gracefully
    bool result = m_eventCapture->startRecording(callback);
    EXPECT_FALSE(result) << "Recording should fail on non-Windows platforms";
    EXPECT_FALSE(m_eventCapture->isRecording());
    EXPECT_FALSE(m_eventCapture->getLastError().empty());
}
#endif

// Test class teardown
TEST_F(WindowsEventCaptureTest, DestructorStopsRecording)
{
    auto callback = [](std::unique_ptr<Event> event)
    {
        (void) event; // Suppress unused parameter warning
        // Empty callback
    };

#ifdef _WIN32
    if (m_eventCapture->startRecording(callback))
    {
        EXPECT_TRUE(m_eventCapture->isRecording());
    }
#endif

    // Destructor should clean up properly
    m_eventCapture.reset();

    // If we get here without crashing, the test passes
    SUCCEED();
}
