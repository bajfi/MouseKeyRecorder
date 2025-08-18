#include <gtest/gtest.h>
#include <QApplication>
#include <QSignalSpy>
#include <QTimer>
#include <QTest>
#include <QShortcut>
#include <QKeyEvent>
#include <QSystemTrayIcon>
#include "../../src/gui/MainWindow.hpp"
#include "../../src/application/MouseRecorderApp.hpp"
#include "../../src/gui/TestUtils.hpp"

namespace MouseRecorder::Tests::GUI
{

class SystemTrayKeyboardShortcutTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create application instance
        m_app = std::make_unique<Application::MouseRecorderApp>();

        // Initialize in headless mode for testing
        ASSERT_TRUE(m_app->initialize("", true))
            << "Failed to initialize MouseRecorderApp";

        // Create main window
        m_window = std::make_unique<::MouseRecorder::GUI::MainWindow>(*m_app);
        m_window->show();

        // Process events to ensure GUI is ready
        QApplication::processEvents();
    }

    void TearDown() override
    {
        m_window.reset();
        m_app.reset();
    }

    std::unique_ptr<Application::MouseRecorderApp> m_app;
    std::unique_ptr<::MouseRecorder::GUI::MainWindow> m_window;
};

TEST_F(SystemTrayKeyboardShortcutTest, StartRecordingShortcutWorks)
{
    ASSERT_FALSE(m_app->getEventRecorder().isRecording());

    // Simulate Ctrl+R shortcut key press
    QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_R, Qt::ControlModifier);
    QApplication::sendEvent(m_window.get(), &keyPress);

    // Process events to handle shortcut activation
    QApplication::processEvents();

    // Check if recording started
    EXPECT_TRUE(m_app->getEventRecorder().isRecording());
}

TEST_F(SystemTrayKeyboardShortcutTest, StopRecordingShortcutWorks)
{
    // First start recording
    ASSERT_TRUE(m_app->getEventRecorder().startRecording(
        [](std::unique_ptr<Core::Event>) {}));
    ASSERT_TRUE(m_app->getEventRecorder().isRecording());

    // Simulate Ctrl+Shift+R shortcut key press
    QKeyEvent keyPress(
        QEvent::KeyPress, Qt::Key_R, Qt::ControlModifier | Qt::ShiftModifier);
    QApplication::sendEvent(m_window.get(), &keyPress);

    // Process events to handle shortcut activation
    QApplication::processEvents();

    // Check if recording stopped
    EXPECT_FALSE(m_app->getEventRecorder().isRecording());
}

TEST_F(SystemTrayKeyboardShortcutTest, DuplicateStartRecordingIgnored)
{
    // Start recording first time
    ASSERT_TRUE(m_app->getEventRecorder().startRecording(
        [](std::unique_ptr<Core::Event>) {}));
    ASSERT_TRUE(m_app->getEventRecorder().isRecording());

    // Try to start recording again via shortcut
    QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_R, Qt::ControlModifier);
    QApplication::sendEvent(m_window.get(), &keyPress);
    QApplication::processEvents();

    // Should still be recording (no error)
    EXPECT_TRUE(m_app->getEventRecorder().isRecording());
}

TEST_F(SystemTrayKeyboardShortcutTest, StopRecordingWhenNotRecordingIgnored)
{
    // Ensure not recording
    ASSERT_FALSE(m_app->getEventRecorder().isRecording());

    // Try to stop recording via shortcut
    QKeyEvent keyPress(
        QEvent::KeyPress, Qt::Key_R, Qt::ControlModifier | Qt::ShiftModifier);
    QApplication::sendEvent(m_window.get(), &keyPress);
    QApplication::processEvents();

    // Should still not be recording (no error)
    EXPECT_FALSE(m_app->getEventRecorder().isRecording());
}

class SystemTrayTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Skip test if system tray is not available
        if (!QSystemTrayIcon::isSystemTrayAvailable())
        {
            GTEST_SKIP() << "System tray not available on this system";
        }

        m_app = std::make_unique<Application::MouseRecorderApp>();
        ASSERT_TRUE(m_app->initialize("", true));
        m_window = std::make_unique<::MouseRecorder::GUI::MainWindow>(*m_app);
        m_window->show();
        QApplication::processEvents();
    }

    void TearDown() override
    {
        m_window.reset();
        m_app.reset();
    }

    std::unique_ptr<Application::MouseRecorderApp> m_app;
    std::unique_ptr<::MouseRecorder::GUI::MainWindow> m_window;
};
};

TEST_F(SystemTrayTest, MinimizeToTrayWorks)
{
    ASSERT_TRUE(m_window->isVisible());

    m_window->minimizeToTray();
    QApplication::processEvents();

    EXPECT_FALSE(m_window->isVisible());
}

TEST_F(SystemTrayTest, RestoreFromTrayWorks)
{
    // First minimize to tray
    m_window->minimizeToTray();
    QApplication::processEvents();
    ASSERT_FALSE(m_window->isVisible());

    // Then restore
    m_window->restoreFromTray();
    QApplication::processEvents();

    EXPECT_TRUE(m_window->isVisible());
}

TEST_F(SystemTrayTest, AutoMinimizeOnRecordingStart)
{
    // Enable auto-minimize
    m_app->getConfiguration().setBool("ui.auto_minimize_on_record", true);

    ASSERT_TRUE(m_window->isVisible());

    // Start recording
    ASSERT_TRUE(m_app->getEventRecorder().startRecording(
        [](std::unique_ptr<Core::Event>) {}));

    // Wait for auto-minimize timer (500ms delay)
    QTimer timer;
    timer.setSingleShot(true);
    QSignalSpy timerSpy(&timer, &QTimer::timeout);
    timer.start(600); // Wait a bit longer than the 500ms delay
    ASSERT_TRUE(timerSpy.wait(1000));

    QApplication::processEvents();

    EXPECT_FALSE(m_window->isVisible());
}

TEST_F(SystemTrayTest, AutoRestoreOnRecordingStop)
{
    // Enable auto-minimize and start recording
    m_app->getConfiguration().setBool("ui.auto_minimize_on_record", true);
    ASSERT_TRUE(m_app->getEventRecorder().startRecording(
        [](std::unique_ptr<Core::Event>) {}));

    // Wait for auto-minimize
    QTimer timer;
    timer.setSingleShot(true);
    QSignalSpy timerSpy(&timer, &QTimer::timeout);
    timer.start(600);
    ASSERT_TRUE(timerSpy.wait(1000));
    QApplication::processEvents();
    ASSERT_FALSE(m_window->isVisible());

    // Stop recording
    m_app->getEventRecorder().stopRecording();

    // Wait for auto-restore timer (200ms delay)
    timer.start(300);
    QSignalSpy timerSpy2(&timer, &QTimer::timeout);
    ASSERT_TRUE(timerSpy2.wait(1000));
    QApplication::processEvents();

    EXPECT_TRUE(m_window->isVisible());
}

} // namespace MouseRecorder::Tests::GUI
