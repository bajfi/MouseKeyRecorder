// Copyright (c) 2025 JackLee
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <gtest/gtest.h>
#include <QApplication>
#include <QTimer>
#include <QTest>
#include "../src/gui/MainWindow.hpp"
#include "../src/application/MouseRecorderApp.hpp"

namespace MouseRecorder::Tests
{

class SystemTrayAndShortcutTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create application instance
        m_app = std::make_unique<Application::MouseRecorderApp>();

        // Initialize in headless mode
        ASSERT_TRUE(m_app->initialize("", true))
            << "Failed to initialize MouseRecorderApp";

        // Create main window
        m_window = std::make_unique<GUI::MainWindow>(*m_app);
        m_window->show();

        QApplication::processEvents();
    }

    void TearDown() override
    {
        m_window.reset();
        m_app.reset();
    }

    std::unique_ptr<Application::MouseRecorderApp> m_app;
    std::unique_ptr<GUI::MainWindow> m_window;
};

TEST_F(SystemTrayAndShortcutTest, BasicFunctionality)
{
    // Test that recording can be started and stopped
    EXPECT_FALSE(m_app->getEventRecorder().isRecording());

    // Start recording programmatically
    bool started = m_app->getEventRecorder().startRecording(
        [](std::unique_ptr<Core::Event>) {});
    EXPECT_TRUE(started);
    EXPECT_TRUE(m_app->getEventRecorder().isRecording());

    // Stop recording
    m_app->getEventRecorder().stopRecording();
    EXPECT_FALSE(m_app->getEventRecorder().isRecording());
}

TEST_F(SystemTrayAndShortcutTest, SystemTrayBasicOperations)
{
    // Only test if system tray is available
    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        GTEST_SKIP() << "System tray not available";
    }

    // Test minimize and restore operations
    EXPECT_TRUE(m_window->isVisible());

    m_window->minimizeToTray();
    QApplication::processEvents();
    EXPECT_FALSE(m_window->isVisible());

    m_window->restoreFromTray();
    QApplication::processEvents();
    EXPECT_TRUE(m_window->isVisible());
}

TEST_F(SystemTrayAndShortcutTest, RecordingStateValidation)
{
    // Test duplicate recording prevention
    EXPECT_FALSE(m_app->getEventRecorder().isRecording());

    // Start recording
    EXPECT_TRUE(m_app->getEventRecorder().startRecording(
        [](std::unique_ptr<Core::Event>) {}));
    EXPECT_TRUE(m_app->getEventRecorder().isRecording());

    // Try to start recording again (should be handled gracefully)
    // This should not cause errors or crashes
    EXPECT_FALSE(m_app->getEventRecorder().startRecording(
        [](std::unique_ptr<Core::Event>) {}));
    EXPECT_TRUE(m_app->getEventRecorder().isRecording()); // Still recording

    // Stop recording
    m_app->getEventRecorder().stopRecording();
    EXPECT_FALSE(m_app->getEventRecorder().isRecording());

    // Try to stop again (should be handled gracefully)
    m_app->getEventRecorder().stopRecording(); // Should not crash
    EXPECT_FALSE(m_app->getEventRecorder().isRecording());
}

} // namespace MouseRecorder::Tests
