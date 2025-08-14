#include <gtest/gtest.h>
#include <QApplication>
#include <QTest>
#include <QPushButton>
#include <QTimer>
#include "../../src/gui/RecordingWidget.hpp"
#include "../../src/gui/MainWindow.hpp"
#include "../../src/application/MouseRecorderApp.hpp"
#include "../../src/gui/TestUtils.hpp"

namespace MouseRecorder::Tests::GUI
{

class UIStateSynchronizationTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        m_app = std::make_unique<Application::MouseRecorderApp>();
        ASSERT_TRUE(m_app->initialize("", true));
        m_window = std::make_unique<::MouseRecorder::GUI::MainWindow>(*m_app);
        m_window->show();
        QApplication::processEvents();

        // Get reference to the recording widget
        m_recordingWidget =
          m_window->findChild<::MouseRecorder::GUI::RecordingWidget*>();
        ASSERT_NE(m_recordingWidget, nullptr);
    }

    void TearDown() override
    {
        m_window.reset();
        m_app.reset();
    }

    std::unique_ptr<Application::MouseRecorderApp> m_app;
    std::unique_ptr<::MouseRecorder::GUI::MainWindow> m_window;
    ::MouseRecorder::GUI::RecordingWidget* m_recordingWidget{nullptr};
};

TEST_F(UIStateSynchronizationTest, ButtonStatesInitiallCorrect)
{
    auto startButton =
      m_recordingWidget->findChild<QPushButton*>("startRecordingButton");
    auto stopButton =
      m_recordingWidget->findChild<QPushButton*>("stopRecordingButton");

    ASSERT_NE(startButton, nullptr);
    ASSERT_NE(stopButton, nullptr);

    // Initially start should be enabled, stop should be disabled
    EXPECT_TRUE(startButton->isEnabled());
    EXPECT_FALSE(stopButton->isEnabled());
}

TEST_F(UIStateSynchronizationTest, ButtonStatesUpdateOnRecordingStart)
{
    auto startButton =
      m_recordingWidget->findChild<QPushButton*>("startRecordingButton");
    auto stopButton =
      m_recordingWidget->findChild<QPushButton*>("stopRecordingButton");

    // Start recording via MainWindow (simulating shortcut)
    ASSERT_TRUE(m_app->getEventRecorder().startRecording(
      [](std::unique_ptr<Core::Event>) {}
    ));
    m_recordingWidget->startRecordingUI();

    QApplication::processEvents();

    // Start should be disabled, stop should be enabled
    EXPECT_FALSE(startButton->isEnabled());
    EXPECT_TRUE(stopButton->isEnabled());
}

TEST_F(UIStateSynchronizationTest, ButtonStatesUpdateOnRecordingStop)
{
    auto startButton =
      m_recordingWidget->findChild<QPushButton*>("startRecordingButton");
    auto stopButton =
      m_recordingWidget->findChild<QPushButton*>("stopRecordingButton");

    // First start recording
    ASSERT_TRUE(m_app->getEventRecorder().startRecording(
      [](std::unique_ptr<Core::Event>) {}
    ));
    m_recordingWidget->startRecordingUI();
    QApplication::processEvents();

    ASSERT_FALSE(startButton->isEnabled());
    ASSERT_TRUE(stopButton->isEnabled());

    // Then stop recording
    m_app->getEventRecorder().stopRecording();
    m_recordingWidget->stopRecordingUI();
    QApplication::processEvents();

    // Start should be enabled, stop should be disabled
    EXPECT_TRUE(startButton->isEnabled());
    EXPECT_FALSE(stopButton->isEnabled());
}

TEST_F(UIStateSynchronizationTest, SetRecordingStateWorks)
{
    auto startButton =
      m_recordingWidget->findChild<QPushButton*>("startRecordingButton");
    auto stopButton =
      m_recordingWidget->findChild<QPushButton*>("stopRecordingButton");

    // Test setting recording state to true
    m_recordingWidget->setRecordingState(true);
    QApplication::processEvents();

    EXPECT_FALSE(startButton->isEnabled());
    EXPECT_TRUE(stopButton->isEnabled());

    // Test setting recording state to false
    m_recordingWidget->setRecordingState(false);
    QApplication::processEvents();

    EXPECT_TRUE(startButton->isEnabled());
    EXPECT_FALSE(stopButton->isEnabled());
}

TEST_F(UIStateSynchronizationTest, RecordingTimerStartsWithRecordingState)
{
    // Timer should not be active initially
    auto timer = m_recordingWidget->findChild<QTimer*>();
    if (timer)
    {
        EXPECT_FALSE(timer->isActive());
    }

    // Start recording state
    m_recordingWidget->setRecordingState(true);
    QApplication::processEvents();

    // Timer should now be active (if it exists)
    if (timer)
    {
        EXPECT_TRUE(timer->isActive());
    }

    // Stop recording state
    m_recordingWidget->setRecordingState(false);
    QApplication::processEvents();

    // Timer should be stopped
    if (timer)
    {
        EXPECT_FALSE(timer->isActive());
    }
}

} // namespace MouseRecorder::Tests::GUI
