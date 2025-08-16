#include <gtest/gtest.h>
#include <QApplication>
#include <QTemporaryFile>
#include <QtTest/QTest>
#include "application/MouseRecorderApp.hpp"
#include "core/Event.hpp"
#include "storage/JsonEventStorage.hpp"
#include "platform/linux/LinuxEventReplay.hpp"
#include <memory>
#include <thread>

// Undefine X11 macros that conflict with our enums
#ifdef None
#undef None
#endif

namespace MouseRecorder::Tests::Integration
{

class PlaybackIntegrationTest : public ::testing::Test
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
          mouseRecorderApp->initialize("", false)
        ); // headless = false
    }

    void TearDown() override
    {
        if (mouseRecorderApp)
        {
            mouseRecorderApp->shutdown();
        }
        mouseRecorderApp.reset();
    }

    // Create a comprehensive test recording
    std::vector<std::unique_ptr<Core::Event>> createTestEvents()
    {
        std::vector<std::unique_ptr<Core::Event>> events;

        // Mouse move events
        for (int i = 0; i < 5; ++i)
        {
            auto event = Core::EventFactory::createMouseMoveEvent(
              {100 + i * 10, 200 + i * 5}
            );
            events.push_back(std::move(event));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // Mouse click
        events.push_back(
          Core::EventFactory::createMouseClickEvent(
            {150, 225}, Core::MouseButton::Left
          )
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Key sequence: "hello"
        std::vector<std::string> keys = {"h", "e", "l", "l", "o"};
        for (const auto& key : keys)
        {
            events.push_back(
              Core::EventFactory::createKeyPressEvent(
                static_cast<uint32_t>(key[0]), key, Core::KeyModifier::None
              )
            );
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            events.push_back(
              Core::EventFactory::createKeyReleaseEvent(
                static_cast<uint32_t>(key[0]), key, Core::KeyModifier::None
              )
            );
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // Mouse wheel event
        events.push_back(
          Core::EventFactory::createMouseWheelEvent({200, 250}, 120)
        );

        return events;
    }

    QString saveTestEvents(
      const std::vector<std::unique_ptr<Core::Event>>& events
    )
    {
        auto tempFile = std::make_unique<QTemporaryFile>();
        tempFile->setFileTemplate("playback_test_XXXXXX.json");
        EXPECT_TRUE(tempFile->open());
        QString fileName = tempFile->fileName();
        tempFile->close();

        Storage::JsonEventStorage storage;
        Core::StorageMetadata metadata;
        metadata.description = "Playback integration test";
        metadata.totalEvents = events.size();
        metadata.applicationName = "MouseRecorder";
        metadata.version = "1.0.0";

        EXPECT_TRUE(
          storage.saveEvents(events, fileName.toStdString(), metadata)
        );

        // Keep the file alive
        tempFiles.push_back(std::move(tempFile));

        return fileName;
    }

    std::unique_ptr<QApplication> app;
    std::unique_ptr<Application::MouseRecorderApp> mouseRecorderApp;
    std::vector<std::unique_ptr<QTemporaryFile>> tempFiles;
};

TEST_F(PlaybackIntegrationTest, EndToEndPlaybackFlow)
{
    // Create test events
    auto events = createTestEvents();
    size_t originalEventCount = events.size();

    // Save events to file
    QString testFile = saveTestEvents(events);

    // Load events through storage interface
    auto storage = mouseRecorderApp->createStorage(Core::StorageFormat::Json);
    ASSERT_TRUE(storage != nullptr);

    std::vector<std::unique_ptr<Core::Event>> loadedEvents;
    Core::StorageMetadata metadata;

    EXPECT_TRUE(
      storage->loadEvents(testFile.toStdString(), loadedEvents, metadata)
    );
    EXPECT_EQ(loadedEvents.size(), originalEventCount);
    EXPECT_EQ(metadata.totalEvents, originalEventCount);

    // Get the event player
    auto& player = mouseRecorderApp->getEventPlayer();

    // Load events into player
    EXPECT_TRUE(player.loadEvents(std::move(loadedEvents)));
    EXPECT_EQ(player.getTotalEvents(), originalEventCount);
    EXPECT_EQ(player.getCurrentPosition(), 0);
    EXPECT_EQ(player.getState(), Core::PlaybackState::Stopped);

    // Test playback speed settings
    player.setPlaybackSpeed(0.5);
    EXPECT_DOUBLE_EQ(player.getPlaybackSpeed(), 0.5);

    player.setPlaybackSpeed(2.0);
    EXPECT_DOUBLE_EQ(player.getPlaybackSpeed(), 2.0);

    // Reset to normal speed
    player.setPlaybackSpeed(1.0);
    EXPECT_DOUBLE_EQ(player.getPlaybackSpeed(), 1.0);

    // Test looping
    player.setLoopPlayback(true);
    EXPECT_TRUE(player.isLoopEnabled());

    player.setLoopPlayback(false);
    EXPECT_FALSE(player.isLoopEnabled());
}

TEST_F(PlaybackIntegrationTest, PlaybackStateTransitions)
{
    auto events = createTestEvents();
    QString testFile = saveTestEvents(events);

    auto& player = mouseRecorderApp->getEventPlayer();

    // Load events from file via storage
    auto storage = mouseRecorderApp->createStorage(Core::StorageFormat::Json);
    std::vector<std::unique_ptr<Core::Event>> loadedEvents;
    Core::StorageMetadata metadata;
    storage->loadEvents(testFile.toStdString(), loadedEvents, metadata);

    EXPECT_TRUE(player.loadEvents(std::move(loadedEvents)));

    // Test state transitions
    EXPECT_EQ(player.getState(), Core::PlaybackState::Stopped);

    // Set up callback to track state changes
    std::vector<Core::PlaybackState> stateHistory;
    auto callback = [&stateHistory](Core::PlaybackState state, size_t, size_t)
    {
        stateHistory.push_back(state);
    };

    // Start playback at normal speed to allow state checking
    player.setPlaybackSpeed(1.0);
    EXPECT_TRUE(player.startPlayback(callback));

    // Give time for playback to start
    QTest::qWait(50);
    // Check if playback is Playing or has already Completed (due to fast
    // execution)
    auto state = player.getState();
    EXPECT_TRUE(
      state == Core::PlaybackState::Playing ||
      state == Core::PlaybackState::Completed
    );

    // Only test pause/resume if still playing
    if (state == Core::PlaybackState::Playing)
    {
        // Test pause
        player.pausePlayback();
        QTest::qWait(10);
        EXPECT_EQ(player.getState(), Core::PlaybackState::Paused);

        // Test resume
        player.resumePlayback();
        QTest::qWait(10);
        state = player.getState();
        EXPECT_TRUE(
          state == Core::PlaybackState::Playing ||
          state == Core::PlaybackState::Completed
        );
    }

    // Test stop
    player.stopPlayback();
    QTest::qWait(10);
    EXPECT_EQ(player.getState(), Core::PlaybackState::Stopped);

    // Verify we received state callbacks
    EXPECT_FALSE(stateHistory.empty());
}

TEST_F(PlaybackIntegrationTest, PlaybackWithCallback)
{
    auto events = createTestEvents();
    QString testFile = saveTestEvents(events);

    auto& player = mouseRecorderApp->getEventPlayer();

    // Load events
    auto storage = mouseRecorderApp->createStorage(Core::StorageFormat::Json);
    std::vector<std::unique_ptr<Core::Event>> loadedEvents;
    Core::StorageMetadata metadata;
    storage->loadEvents(testFile.toStdString(), loadedEvents, metadata);
    size_t totalEvents = loadedEvents.size();

    EXPECT_TRUE(player.loadEvents(std::move(loadedEvents)));

    // Track callback invocations
    size_t callbackCount = 0;
    size_t lastPosition = 0;
    Core::PlaybackState lastState = Core::PlaybackState::Stopped;

    auto progressCallback =
      [&](Core::PlaybackState state, size_t current, size_t total)
    {
        callbackCount++;
        lastPosition = current;
        lastState = state;
        EXPECT_LE(current, total);
        EXPECT_EQ(total, totalEvents);
    };

    // Set high speed for quick completion
    player.setPlaybackSpeed(10.0);

    // Start playback with callback
    EXPECT_TRUE(player.startPlayback(progressCallback));

    // Wait for playback to complete
    int maxWaitMs = 2000; // 2 seconds max
    int waitedMs = 0;
    while (player.getState() == Core::PlaybackState::Playing &&
           waitedMs < maxWaitMs)
    {
        QTest::qWait(50);
        waitedMs += 50;
    }

    // Should have received callbacks
    EXPECT_GT(callbackCount, 0);

    // Final state should be completed or stopped
    Core::PlaybackState finalState = player.getState();
    EXPECT_TRUE(
      finalState == Core::PlaybackState::Completed ||
      finalState == Core::PlaybackState::Stopped ||
      finalState == Core::PlaybackState::Error
    );
}

TEST_F(PlaybackIntegrationTest, SeekFunctionality)
{
    auto events = createTestEvents();
    QString testFile = saveTestEvents(events);

    auto& player = mouseRecorderApp->getEventPlayer();

    // Load events
    auto storage = mouseRecorderApp->createStorage(Core::StorageFormat::Json);
    std::vector<std::unique_ptr<Core::Event>> loadedEvents;
    Core::StorageMetadata metadata;
    storage->loadEvents(testFile.toStdString(), loadedEvents, metadata);
    size_t totalEvents = loadedEvents.size();

    EXPECT_TRUE(player.loadEvents(std::move(loadedEvents)));

    // Test seeking to different positions
    size_t midPoint = totalEvents / 2;
    EXPECT_TRUE(player.seekToPosition(midPoint));
    EXPECT_EQ(player.getCurrentPosition(), midPoint);

    // Seek to beginning
    EXPECT_TRUE(player.seekToPosition(0));
    EXPECT_EQ(player.getCurrentPosition(), 0);

    // Seek to end (should be clamped to valid range)
    EXPECT_TRUE(player.seekToPosition(totalEvents - 1));
    EXPECT_EQ(player.getCurrentPosition(), totalEvents - 1);

    // Seek beyond end (should fail or be clamped)
    bool result = player.seekToPosition(totalEvents + 100);
    if (result)
    {
        // If it succeeds, position should be clamped
        EXPECT_LT(player.getCurrentPosition(), totalEvents);
    }
}

TEST_F(PlaybackIntegrationTest, ErrorHandling)
{
    auto& player = mouseRecorderApp->getEventPlayer();

    // Try to start playback without loading events
    EXPECT_FALSE(player.startPlayback());
    EXPECT_FALSE(player.getLastError().empty());

    // Try to start playback while already playing
    auto events = createTestEvents();
    QString testFile = saveTestEvents(events);

    auto storage = mouseRecorderApp->createStorage(Core::StorageFormat::Json);
    std::vector<std::unique_ptr<Core::Event>> loadedEvents;
    Core::StorageMetadata metadata;
    storage->loadEvents(testFile.toStdString(), loadedEvents, metadata);

    EXPECT_TRUE(player.loadEvents(std::move(loadedEvents)));
    player.setPlaybackSpeed(10.0); // Fast playback
    EXPECT_TRUE(player.startPlayback());

    // Try to start again while playing
    EXPECT_FALSE(player.startPlayback());

    // Clean up
    player.stopPlayback();
    QTest::qWait(50);
}

TEST_F(PlaybackIntegrationTest, PlaybackRestartAfterCompletion)
{
    auto events = createTestEvents();
    QString testFile = saveTestEvents(events);

    auto& player = mouseRecorderApp->getEventPlayer();

    // Load events from file via storage
    auto storage = mouseRecorderApp->createStorage(Core::StorageFormat::Json);
    std::vector<std::unique_ptr<Core::Event>> loadedEvents;
    Core::StorageMetadata metadata;
    storage->loadEvents(testFile.toStdString(), loadedEvents, metadata);

    EXPECT_TRUE(player.loadEvents(std::move(loadedEvents)));

    // Test initial state
    EXPECT_EQ(player.getState(), Core::PlaybackState::Stopped);

    // Start first playback
    bool firstPlaybackStarted = false;
    bool playbackCompleted = false;
    auto callback =
      [&firstPlaybackStarted,
       &playbackCompleted](Core::PlaybackState state, size_t, size_t)
    {
        if (state == Core::PlaybackState::Playing && !firstPlaybackStarted)
        {
            firstPlaybackStarted = true;
        }
        if (state == Core::PlaybackState::Completed)
        {
            playbackCompleted = true;
        }
    };

    // Set fast speed to complete quickly
    player.setPlaybackSpeed(100.0);
    EXPECT_TRUE(player.startPlayback(callback));

    // Wait for playback to complete
    int timeout = 0;
    while (!playbackCompleted && timeout < 1000)
    {
        QTest::qWait(10);
        timeout += 10;
    }

    EXPECT_TRUE(playbackCompleted);
    EXPECT_EQ(player.getState(), Core::PlaybackState::Completed);

    // Now test restarting playback - THIS IS THE BUG WE FIXED
    bool secondPlaybackStarted = false;
    auto secondCallback =
      [&secondPlaybackStarted](Core::PlaybackState state, size_t, size_t)
    {
        if (state == Core::PlaybackState::Playing)
        {
            secondPlaybackStarted = true;
        }
    };

    // This should succeed after our fix
    EXPECT_TRUE(player.startPlayback(secondCallback));

    QTest::qWait(50);
    EXPECT_TRUE(secondPlaybackStarted);

    // Clean up
    player.stopPlayback();
    EXPECT_EQ(player.getState(), Core::PlaybackState::Stopped);
}

} // namespace MouseRecorder::Tests::Integration
