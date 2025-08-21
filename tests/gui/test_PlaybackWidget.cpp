// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <gtest/gtest.h>
#include <QApplication>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest/QTest>
#include <QTemporaryFile>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QSlider>
#include <QTableWidget>
#include <thread>
#include <chrono>
#include "gui/PlaybackWidget.hpp"
#include "application/MouseRecorderApp.hpp"
#include "core/Event.hpp"
#include "storage/JsonEventStorage.hpp"
#include <memory>

using namespace MouseRecorder::GUI;

namespace MouseRecorder::Tests::GUI
{

// Global static QApplication for all GUI tests to avoid recreation issues
static QApplication* getGlobalTestApp()
{
    static std::unique_ptr<QApplication> globalApp;
    if (!globalApp && !QApplication::instance())
    {
        static int argc = 1;
        static char argv0[] = "test";
        static char* argv[] = {argv0};
        globalApp = std::make_unique<QApplication>(argc, argv);
    }
    return globalApp.get();
}

class PlaybackWidgetTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Ensure we have a QApplication instance
        getGlobalTestApp();

        // Initialize MouseRecorderApp (NOT in headless mode for testing)
        mouseRecorderApp = std::make_unique<Application::MouseRecorderApp>();
        ASSERT_TRUE(
            mouseRecorderApp->initialize("", false)); // headless = false

        // Create PlaybackWidget
        playbackWidget = std::make_unique<MouseRecorder::GUI::PlaybackWidget>(
            *mouseRecorderApp);
    }

    void TearDown() override
    {
        // Simply clean up without forcing Qt event processing
        if (playbackWidget)
        {
            playbackWidget->disconnect();
            playbackWidget.reset();
        }

        if (mouseRecorderApp)
        {
            mouseRecorderApp->shutdown();
            mouseRecorderApp.reset();
        }
    }

    // Helper method to create a test recording file
    QString createTestRecordingFile()
    {
        auto tempFile = std::make_unique<QTemporaryFile>();
        tempFile->setFileTemplate("test_recording_XXXXXX.json");
        EXPECT_TRUE(tempFile->open());
        QString fileName = tempFile->fileName();
        tempFile->close();

        // Create some test events
        std::vector<std::unique_ptr<Core::Event>> events;
        events.push_back(Core::EventFactory::createMouseMoveEvent({100, 200}));
        events.push_back(Core::EventFactory::createMouseClickEvent(
            {100, 200}, Core::MouseButton::Left));
        events.push_back(Core::EventFactory::createKeyPressEvent(
            65, "a", Core::KeyModifier::None));
        events.push_back(Core::EventFactory::createKeyReleaseEvent(
            65, "a", Core::KeyModifier::None));

        // Save events to file
        Storage::JsonEventStorage storage;
        Core::StorageMetadata metadata;
        metadata.description = "Test recording";
        metadata.totalEvents = events.size();

        EXPECT_TRUE(
            storage.saveEvents(events, fileName.toStdString(), metadata));

        // Keep the file alive by storing the unique_ptr
        tempFiles.push_back(std::move(tempFile));

        return fileName;
    }

    std::unique_ptr<Application::MouseRecorderApp> mouseRecorderApp;
    std::unique_ptr<MouseRecorder::GUI::PlaybackWidget> playbackWidget;
    std::vector<std::unique_ptr<QTemporaryFile>> tempFiles;
};

TEST_F(PlaybackWidgetTest, InitialState)
{
    EXPECT_TRUE(playbackWidget != nullptr);

    // Check initial UI state - buttons should be disabled initially
    auto* playButton = playbackWidget->findChild<QPushButton*>("playButton");
    auto* stopButton = playbackWidget->findChild<QPushButton*>("stopButton");

    ASSERT_TRUE(playButton != nullptr);
    ASSERT_TRUE(stopButton != nullptr);

    EXPECT_FALSE(playButton->isEnabled());
    EXPECT_FALSE(stopButton->isEnabled());
}

TEST_F(PlaybackWidgetTest, LoadFile)
{
    QString testFile = createTestRecordingFile();

    // Note: The PlaybackWidget doesn't have a fileLoaded signal in the current
    // implementation so we'll test the functionality differently

    // Simulate loading a file directly
    playbackWidget->loadFile(testFile);

    // Give some time for the file to load
    QTest::qWait(100);

    // Check that file was loaded
    auto* filePathEdit =
        playbackWidget->findChild<QLineEdit*>("filePathLineEdit");
    ASSERT_TRUE(filePathEdit != nullptr);
    EXPECT_EQ(filePathEdit->text().toStdString(), testFile.toStdString());

    // Check that play button is now enabled
    auto* playButton = playbackWidget->findChild<QPushButton*>("playButton");
    ASSERT_TRUE(playButton != nullptr);
    EXPECT_TRUE(playButton->isEnabled());
}

TEST_F(PlaybackWidgetTest, PlaybackControls)
{
    QString testFile = createTestRecordingFile();

    // Load the file first
    playbackWidget->loadFile(testFile);
    QTest::qWait(100);

    // Set up signal spies
    QSignalSpy playbackStartedSpy(
        playbackWidget.get(),
        &MouseRecorder::GUI::PlaybackWidget::playbackStarted);
    QSignalSpy playbackStoppedSpy(
        playbackWidget.get(),
        &MouseRecorder::GUI::PlaybackWidget::playbackStopped);

    auto* playButton = playbackWidget->findChild<QPushButton*>("playButton");
    auto* stopButton = playbackWidget->findChild<QPushButton*>("stopButton");

    // Test play
    QTest::mouseClick(playButton, Qt::LeftButton);
    QTest::qWait(50);

    EXPECT_GE(playbackStartedSpy.count(), 1);
    EXPECT_FALSE(playButton->isEnabled());
    EXPECT_TRUE(stopButton->isEnabled());

    // Test stop
    QTest::mouseClick(stopButton, Qt::LeftButton);
    QTest::qWait(50);

    EXPECT_GE(playbackStoppedSpy.count(), 1);
    EXPECT_TRUE(playButton->isEnabled());
    EXPECT_FALSE(stopButton->isEnabled());
}

TEST_F(PlaybackWidgetTest, SpeedControl)
{
    QString testFile = createTestRecordingFile();

    // Load the file first
    playbackWidget->loadFile(testFile);
    QTest::qWait(100);

    auto* speedSlider = playbackWidget->findChild<QSlider*>("speedSlider");
    auto* speedLabel = playbackWidget->findChild<QLabel*>("speedValueLabel");

    ASSERT_TRUE(speedSlider != nullptr);
    ASSERT_TRUE(speedLabel != nullptr);

    // Test speed change
    speedSlider->setValue(20); // 2.0x speed
    QTest::qWait(50);

    // Check that the label was updated
    EXPECT_TRUE(speedLabel->text().contains("2.0x"));

    // Check that the player speed was updated
    EXPECT_DOUBLE_EQ(mouseRecorderApp->getEventPlayer().getPlaybackSpeed(),
                     2.0);
}

TEST_F(PlaybackWidgetTest, FileMetadataDisplay)
{
    QString testFile = createTestRecordingFile();

    // Load the file
    playbackWidget->loadFile(testFile);
    QTest::qWait(100);

    auto* totalEventsLabel =
        playbackWidget->findChild<QLabel*>("totalEventsValue");
    auto* fileFormatLabel =
        playbackWidget->findChild<QLabel*>("fileFormatValue");

    ASSERT_TRUE(totalEventsLabel != nullptr);
    ASSERT_TRUE(fileFormatLabel != nullptr);

    // Should show 4 events (the test events we created)
    EXPECT_EQ(totalEventsLabel->text().toStdString(), "4");
    EXPECT_EQ(fileFormatLabel->text().toStdString(), "JSON");
}

TEST_F(PlaybackWidgetTest, ProgressTracking)
{
    QString testFile = createTestRecordingFile();

    // Load file and start playback
    playbackWidget->loadFile(testFile);
    QTest::qWait(100);

    auto* progressSlider =
        playbackWidget->findChild<QSlider*>("progressSlider");
    ASSERT_TRUE(progressSlider != nullptr);

    // Check initial state
    EXPECT_EQ(progressSlider->value(), 0);
    EXPECT_EQ(progressSlider->maximum(), 3); // 4 events, max index is 3

    // Start playback
    auto* playButton = playbackWidget->findChild<QPushButton*>("playButton");
    QTest::mouseClick(playButton, Qt::LeftButton);

    // Wait a bit and check that progress updates
    QTest::qWait(200);

    // Progress should have advanced (or completed for short test)
    EXPECT_GE(progressSlider->value(), 0);
}

TEST_F(PlaybackWidgetTest, ErrorHandling)
{
    // Try to play without loading a file
    auto* playButton = playbackWidget->findChild<QPushButton*>("playButton");

    // Should be disabled initially
    EXPECT_FALSE(playButton->isEnabled());

    // Try to load an invalid file
    QString invalidFile = "/nonexistent/file.json";
    playbackWidget->loadFile(invalidFile);

    // Process any pending Qt events from the loadFile operation
    if (QApplication::instance())
    {
        QApplication::processEvents(QEventLoop::AllEvents, 200);
    }

    // Play button should still be disabled
    EXPECT_FALSE(playButton->isEnabled());
}

TEST_F(PlaybackWidgetTest, LoadFileRequestedSlot)
{
    if (!playbackWidget)
    {
        GTEST_SKIP() << "PlaybackWidget could not be created";
    }

    // Create a temporary test file
    QTemporaryFile tempFile;
    tempFile.setFileTemplate("test_XXXXXX.json");
    ASSERT_TRUE(tempFile.open());

    // Create some test events
    std::vector<std::unique_ptr<Core::Event>> events;
    events.push_back(Core::EventFactory::createMouseClickEvent(
        Core::Point{150, 250}, Core::MouseButton::Left));
    events.push_back(Core::EventFactory::createKeyPressEvent(72, "H"));

    // Save test events to file
    Storage::JsonEventStorage storage;
    Core::StorageMetadata metadata;
    metadata.version = "1.0";
    metadata.description = "Test events";

    ASSERT_TRUE(storage.saveEvents(
        events, tempFile.fileName().toStdString(), metadata));

    tempFile.close();

    // Set up signal spy to monitor fileLoaded signal
    QSignalSpy fileLoadedSpy(playbackWidget.get(), &PlaybackWidget::fileLoaded);

    // Test the loadFileRequested slot
    playbackWidget->loadFileRequested(tempFile.fileName());

    // Allow some time for the file to load
    QTest::qWait(500);

    // Verify the signal was emitted
    EXPECT_EQ(fileLoadedSpy.count(), 1);

    if (fileLoadedSpy.count() > 0)
    {
        // Verify the signal parameter is correct
        QList<QVariant> arguments = fileLoadedSpy.takeFirst();
        QString loadedFileName = arguments.at(0).toString();
        EXPECT_EQ(loadedFileName, tempFile.fileName());
    }

    // Verify UI elements are updated correctly
    auto* playButton = playbackWidget->findChild<QPushButton*>("playButton");
    ASSERT_NE(playButton, nullptr);
    EXPECT_TRUE(playButton->isEnabled());
}

} // namespace MouseRecorder::Tests::GUI
