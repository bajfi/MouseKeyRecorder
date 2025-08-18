#include <gtest/gtest.h>
#include <QApplication>
#include <QTemporaryFile>
#include <QtTest/QTest>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QSlider>
#include <QSignalSpy>
#include "application/MouseRecorderApp.hpp"
#include "gui/MainWindow.hpp"
#include "gui/PlaybackWidget.hpp"
#include "core/Event.hpp"
#include "storage/JsonEventStorage.hpp"
#include <memory>

namespace MouseRecorder::Tests::E2E
{

class EndToEndPlaybackTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Initialize QApplication if not already done
        if (!QApplication::instance())
        {
            static int argc = 1;
            static char argv0[] = "test";
            static char* argv[] = {argv0};
            app = std::make_unique<QApplication>(argc, argv);
        }

        // Initialize MouseRecorderApp (NOT headless for testing)
        mouseRecorderApp = std::make_unique<Application::MouseRecorderApp>();
        ASSERT_TRUE(
            mouseRecorderApp->initialize("", false)); // headless = false
    }

    void TearDown() override
    {
        if (mouseRecorderApp)
        {
            mouseRecorderApp->shutdown();
        }
        mouseRecorderApp.reset();
    }

    QString createSimpleTestFile()
    {
        auto tempFile = std::make_unique<QTemporaryFile>();
        tempFile->setFileTemplate("e2e_test_XXXXXX.json");
        EXPECT_TRUE(tempFile->open());
        QString fileName = tempFile->fileName();
        tempFile->close();

        // Create simple test events
        std::vector<std::unique_ptr<Core::Event>> events;
        events.push_back(Core::EventFactory::createMouseMoveEvent({100, 100}));
        events.push_back(Core::EventFactory::createMouseClickEvent(
            {100, 100}, Core::MouseButton::Left));
        events.push_back(Core::EventFactory::createKeyPressEvent(
            32, "space", Core::KeyModifier::None));

        Storage::JsonEventStorage storage;
        Core::StorageMetadata metadata;
        metadata.description = "E2E test";

        EXPECT_TRUE(
            storage.saveEvents(events, fileName.toStdString(), metadata));

        tempFiles.push_back(std::move(tempFile));
        return fileName;
    }

    std::unique_ptr<QApplication> app;
    std::unique_ptr<Application::MouseRecorderApp> mouseRecorderApp;
    std::vector<std::unique_ptr<QTemporaryFile>> tempFiles;
};

TEST_F(EndToEndPlaybackTest, CompletePlaybackWorkflow)
{
    // Create a test recording file
    QString testFile = createSimpleTestFile();
    ASSERT_FALSE(testFile.isEmpty());

    // Create PlaybackWidget
    auto playbackWidget =
        std::make_unique<MouseRecorder::GUI::PlaybackWidget>(*mouseRecorderApp);

    // Load the test file
    playbackWidget->loadFile(testFile);
    QTest::qWait(100); // Wait for file to load

    // Verify file was loaded
    auto* filePathEdit =
        playbackWidget->findChild<QLineEdit*>("filePathLineEdit");
    ASSERT_TRUE(filePathEdit != nullptr);
    EXPECT_EQ(filePathEdit->text(), testFile);

    // Verify event count
    auto* totalEventsLabel =
        playbackWidget->findChild<QLabel*>("totalEventsValue");
    ASSERT_TRUE(totalEventsLabel != nullptr);
    EXPECT_EQ(totalEventsLabel->text().toInt(), 3);

    // Get control buttons
    auto* playButton = playbackWidget->findChild<QPushButton*>("playButton");
    auto* stopButton = playbackWidget->findChild<QPushButton*>("stopButton");

    ASSERT_TRUE(playButton != nullptr);
    ASSERT_TRUE(stopButton != nullptr);

    // Should be able to play now
    EXPECT_TRUE(playButton->isEnabled());
    EXPECT_FALSE(stopButton->isEnabled());

    // Set up signal spy
    QSignalSpy playbackStartedSpy(
        playbackWidget.get(),
        &MouseRecorder::GUI::PlaybackWidget::playbackStarted);
    QSignalSpy playbackStoppedSpy(
        playbackWidget.get(),
        &MouseRecorder::GUI::PlaybackWidget::playbackStopped);

    // Start playback at high speed
    auto* speedSlider = playbackWidget->findChild<QSlider*>("speedSlider");
    ASSERT_TRUE(speedSlider != nullptr);
    speedSlider->setValue(50); // 5.0x speed for quick completion

    // Click play
    QTest::mouseClick(playButton, Qt::LeftButton);
    QTest::qWait(50);

    // Verify playback started
    EXPECT_GE(playbackStartedSpy.count(), 1);
    EXPECT_FALSE(playButton->isEnabled());
    EXPECT_TRUE(stopButton->isEnabled());

    // Wait for playback to complete or timeout
    int maxWait = 2000; // 2 seconds
    int waited = 0;
    while (playbackStoppedSpy.count() == 0 && waited < maxWait)
    {
        QTest::qWait(100);
        waited += 100;
    }

    // Should have either completed automatically or we can stop it manually
    if (playbackStoppedSpy.count() == 0)
    {
        // Stop manually if not completed
        QTest::mouseClick(stopButton, Qt::LeftButton);
        QTest::qWait(50);
    }

    // Verify final state
    EXPECT_TRUE(playButton->isEnabled());
    EXPECT_FALSE(stopButton->isEnabled());
}

TEST_F(EndToEndPlaybackTest, AppIntegrationTest)
{
    QString testFile = createSimpleTestFile();

    // Test that the app has properly created the event player
    auto& player = mouseRecorderApp->getEventPlayer();
    EXPECT_EQ(player.getState(), Core::PlaybackState::Stopped);

    // Test loading events through storage
    auto storage = mouseRecorderApp->createStorage(Core::StorageFormat::Json);
    ASSERT_TRUE(storage != nullptr);

    std::vector<std::unique_ptr<Core::Event>> events;
    Core::StorageMetadata metadata;

    EXPECT_TRUE(storage->loadEvents(testFile.toStdString(), events, metadata));
    EXPECT_EQ(events.size(), 3);

    // Load events into player
    EXPECT_TRUE(player.loadEvents(std::move(events)));
    EXPECT_EQ(player.getTotalEvents(), 3);

    // Test playback speed and loop settings
    player.setPlaybackSpeed(5.0); // Fast speed
    EXPECT_DOUBLE_EQ(player.getPlaybackSpeed(), 5.0);

    player.setLoopPlayback(false);
    EXPECT_FALSE(player.isLoopEnabled());

    // Start and quickly stop playback
    EXPECT_TRUE(player.startPlayback());
    QTest::qWait(50);
    // Check if playback is Playing or has already Completed (due to fast
    // execution)
    auto state = player.getState();
    EXPECT_TRUE(state == Core::PlaybackState::Playing ||
                state == Core::PlaybackState::Completed);

    player.stopPlayback();
    QTest::qWait(50);
    EXPECT_EQ(player.getState(), Core::PlaybackState::Stopped);
}

TEST_F(EndToEndPlaybackTest, RecentFilesIntegration)
{
    // Create MainWindow to test recent files functionality
    GUI::MainWindow mainWindow(*mouseRecorderApp);
    mainWindow.show();
    QTest::qWait(100);

    // Create a test file
    QString testFile = createSimpleTestFile();

    // Get references to playback widget and tab widget
    auto* playbackWidget = mainWindow.findChild<GUI::PlaybackWidget*>();
    ASSERT_NE(playbackWidget, nullptr);

    auto* tabWidget = mainWindow.findChild<QTabWidget*>("tabWidget");
    ASSERT_NE(tabWidget, nullptr);

    // Set up signal spies
    QSignalSpy fileLoadRequestedSpy(&mainWindow,
                                    &GUI::MainWindow::fileLoadRequested);
    QSignalSpy fileLoadedSpy(playbackWidget, &GUI::PlaybackWidget::fileLoaded);

    // Initially we should be on Recording tab (index 0)
    EXPECT_EQ(tabWidget->currentIndex(), 0);

    // Emit fileLoadRequested signal to simulate recent file click
    emit mainWindow.fileLoadRequested(testFile);

    // Allow time for file loading
    QTest::qWait(500);

    // Verify signal was received
    EXPECT_EQ(fileLoadRequestedSpy.count(), 1);

    // Verify file was loaded in PlaybackWidget
    EXPECT_EQ(fileLoadedSpy.count(), 1);

    if (fileLoadedSpy.count() > 0)
    {
        QList<QVariant> arguments = fileLoadedSpy.takeFirst();
        QString loadedFileName = arguments.at(0).toString();
        EXPECT_EQ(loadedFileName, testFile);
    }

    // Verify we switched to Playback tab (index 1)
    EXPECT_EQ(tabWidget->currentIndex(), 1);

    // Verify play button is enabled
    auto* playButton = playbackWidget->findChild<QPushButton*>("playButton");
    ASSERT_NE(playButton, nullptr);
    EXPECT_TRUE(playButton->isEnabled());

    // Cleanup
    QFile::remove(testFile);
}

} // namespace MouseRecorder::Tests::E2E
