#include <gtest/gtest.h>
#include "platform/linux/LinuxEventCapture.hpp"
#include "core/Event.hpp"
#include "core/QtConfiguration.hpp"
#include <thread>
#include <chrono>
#include <atomic>
#include <memory>

using namespace MouseRecorder::Platform::Linux;
using namespace MouseRecorder::Core;

class LinuxEventCaptureTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        m_config = std::make_unique<QtConfiguration>();
        m_eventCapture = std::make_unique<LinuxEventCapture>(*m_config);
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

    std::unique_ptr<IConfiguration> m_config;
    std::unique_ptr<LinuxEventCapture> m_eventCapture;
    std::vector<std::unique_ptr<Event>> m_capturedEvents;
    std::atomic<bool> m_callbackCalled{false};
};

TEST_F(LinuxEventCaptureTest, InitialState)
{
    EXPECT_FALSE(m_eventCapture->isRecording());
    EXPECT_TRUE(m_eventCapture->getLastError().empty());
}

TEST_F(LinuxEventCaptureTest, ConfigurationMethods)
{
    // Test configuration setters (these should not throw)
    EXPECT_NO_THROW(m_eventCapture->setCaptureMouseEvents(true));
    EXPECT_NO_THROW(m_eventCapture->setCaptureKeyboardEvents(false));
    EXPECT_NO_THROW(m_eventCapture->setOptimizeMouseMovements(true));
    EXPECT_NO_THROW(m_eventCapture->setMouseMovementThreshold(10));
}

TEST_F(LinuxEventCaptureTest, StartRecordingWithoutCallback)
{
    // Should fail when no callback is provided
    EXPECT_FALSE(m_eventCapture->startRecording(nullptr));
    EXPECT_FALSE(m_eventCapture->getLastError().empty());
    EXPECT_FALSE(m_eventCapture->isRecording());
}

TEST_F(LinuxEventCaptureTest, StartRecordingWithCallback)
{
    auto callback = [this](std::unique_ptr<Event> event)
    {
        m_capturedEvents.push_back(std::move(event));
        m_callbackCalled.store(true);
    };

    // Note: This test might fail in headless environments without X11
    // In CI environments, we would need to mock X11 or skip this test
    bool started = m_eventCapture->startRecording(callback);

    if (started)
    {
        EXPECT_TRUE(m_eventCapture->isRecording());

        // Let it record for a short time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        m_eventCapture->stopRecording();
        EXPECT_FALSE(m_eventCapture->isRecording());
    }
    else
    {
        // If we can't start recording (e.g., no X11), that's OK for this test
        // Just verify the error is reported properly
        EXPECT_FALSE(m_eventCapture->getLastError().empty());
    }
}

TEST_F(LinuxEventCaptureTest, StopRecordingWhenNotRecording)
{
    // Should be safe to call stop when not recording
    EXPECT_NO_THROW(m_eventCapture->stopRecording());
    EXPECT_FALSE(m_eventCapture->isRecording());
}

TEST_F(LinuxEventCaptureTest, DoubleStartRecording)
{
    auto callback = [this](std::unique_ptr<Event> event)
    {
        m_capturedEvents.push_back(std::move(event));
    };

    bool started = m_eventCapture->startRecording(callback);

    if (started)
    {
        // Try to start again while already recording
        EXPECT_FALSE(m_eventCapture->startRecording(callback));
        EXPECT_FALSE(m_eventCapture->getLastError().empty());

        m_eventCapture->stopRecording();
    }
}

TEST_F(LinuxEventCaptureTest, MouseMovementThreshold)
{
    // Test threshold setting with various values
    m_eventCapture->setMouseMovementThreshold(-5);  // Should be clamped to 0
    m_eventCapture->setMouseMovementThreshold(0);   // Should work
    m_eventCapture->setMouseMovementThreshold(10);  // Should work
    m_eventCapture->setMouseMovementThreshold(100); // Should work

    // No way to verify the actual value without access to internals,
    // but at least verify no exceptions are thrown
    SUCCEED();
}

TEST_F(LinuxEventCaptureTest, ShortcutFilteringConfiguration)
{
    // Test that shortcut filtering configuration is properly handled

    // Enable shortcut filtering
    m_config->setBool(ConfigKeys::FILTER_STOP_RECORDING_SHORTCUT, true);
    EXPECT_TRUE(
        m_config->getBool(ConfigKeys::FILTER_STOP_RECORDING_SHORTCUT, false));

    // Disable shortcut filtering
    m_config->setBool(ConfigKeys::FILTER_STOP_RECORDING_SHORTCUT, false);
    EXPECT_FALSE(
        m_config->getBool(ConfigKeys::FILTER_STOP_RECORDING_SHORTCUT, true));

    // Test with custom shortcuts
    m_config->setString(ConfigKeys::SHORTCUT_START_RECORDING, "Ctrl+F1");
    m_config->setString(ConfigKeys::SHORTCUT_STOP_RECORDING, "Ctrl+F2");

    EXPECT_EQ("Ctrl+F1",
              m_config->getString(ConfigKeys::SHORTCUT_START_RECORDING, ""));
    EXPECT_EQ("Ctrl+F2",
              m_config->getString(ConfigKeys::SHORTCUT_STOP_RECORDING, ""));
}
