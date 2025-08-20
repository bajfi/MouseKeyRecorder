#include <gtest/gtest.h>
#include "platform/windows/WindowsEventReplay.hpp"
#include "core/Event.hpp"
#include <memory>
#include <vector>
#include <chrono>
#include <thread>

using namespace MouseRecorder::Platform::Windows;
using namespace MouseRecorder::Core;

class WindowsEventReplayTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        m_eventReplay = std::make_unique<WindowsEventReplay>();
    }

    void TearDown() override
    {
        if (m_eventReplay &&
            m_eventReplay->getState() == PlaybackState::Playing)
        {
            m_eventReplay->stopPlayback();
        }
        m_eventReplay.reset();
    }

    std::unique_ptr<WindowsEventReplay> m_eventReplay;

    std::vector<std::unique_ptr<Event>> createTestEvents()
    {
        std::vector<std::unique_ptr<Event>> events;

        // Create a simple mouse move event
        auto mouseEvent = EventFactory::createMouseMoveEvent(Point{100, 100});
        events.push_back(std::move(mouseEvent));

        // Create a mouse click event
        auto clickEvent = EventFactory::createMouseClickEvent(
            Point{100, 100}, MouseButton::Left);
        events.push_back(std::move(clickEvent));

        return events;
    }
};

TEST_F(WindowsEventReplayTest, ConstructorInitializesCorrectly)
{
    EXPECT_EQ(m_eventReplay->getState(), PlaybackState::Stopped);
    EXPECT_EQ(m_eventReplay->getPlaybackSpeed(), 1.0);
    EXPECT_FALSE(m_eventReplay->isLoopEnabled());
    EXPECT_EQ(m_eventReplay->getLoopCount(), 1);
    EXPECT_EQ(m_eventReplay->getCurrentPosition(), 0);
    EXPECT_EQ(m_eventReplay->getTotalEvents(), 0);
    EXPECT_TRUE(m_eventReplay->getLastError().empty());
}

TEST_F(WindowsEventReplayTest, ConfigurationSettings)
{
    // Test playback speed
    m_eventReplay->setPlaybackSpeed(2.0);
    EXPECT_EQ(m_eventReplay->getPlaybackSpeed(), 2.0);

    m_eventReplay->setPlaybackSpeed(0.5);
    EXPECT_EQ(m_eventReplay->getPlaybackSpeed(), 0.5);

    // Test loop settings
    m_eventReplay->setLoopPlayback(true);
    EXPECT_TRUE(m_eventReplay->isLoopEnabled());

    m_eventReplay->setLoopCount(5);
    EXPECT_EQ(m_eventReplay->getLoopCount(), 5);

    m_eventReplay->setLoopPlayback(false);
    EXPECT_FALSE(m_eventReplay->isLoopEnabled());
}

TEST_F(WindowsEventReplayTest, LoadEvents)
{
    auto events = createTestEvents();
    size_t eventCount = events.size();

#ifdef _WIN32
    // On Windows, this should succeed
    bool result = m_eventReplay->loadEvents(std::move(events));
    EXPECT_TRUE(result) << "Loading events should succeed on Windows";
    EXPECT_EQ(m_eventReplay->getTotalEvents(), eventCount);
#else
    // On non-Windows platforms, this should fail gracefully
    bool result = m_eventReplay->loadEvents(std::move(events));
    EXPECT_FALSE(result)
        << "Loading events should fail on non-Windows platforms";
    EXPECT_EQ(m_eventReplay->getTotalEvents(), 0);
    EXPECT_FALSE(m_eventReplay->getLastError().empty());
#endif
}

#ifdef _WIN32
TEST_F(WindowsEventReplayTest, StartPlaybackWithEvents)
{
    auto events = createTestEvents();

    bool loadResult = m_eventReplay->loadEvents(std::move(events));
    ASSERT_TRUE(loadResult) << "Events should load successfully";

    bool playbackCallbackInvoked = false;
    auto playbackCallback =
        [&](PlaybackState state, size_t current, size_t total)
    {
        playbackCallbackInvoked = true;
        EXPECT_GE(current, 0);
        EXPECT_LE(current, total);
        EXPECT_GT(total, 0);
    };

    bool result = m_eventReplay->startPlayback(playbackCallback);
    EXPECT_TRUE(result) << "Playback should start successfully";

    if (result)
    {
        EXPECT_NE(m_eventReplay->getState(), PlaybackState::Stopped);

        // Wait a short time for playback to potentially start
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Stop playback
        m_eventReplay->stopPlayback();
        EXPECT_EQ(m_eventReplay->getState(), PlaybackState::Stopped);
    }
}

TEST_F(WindowsEventReplayTest, StartPlaybackWithoutEvents)
{
    // Should fail if no events are loaded
    bool result = m_eventReplay->startPlayback();
    EXPECT_FALSE(result) << "Playback should fail without loaded events";
    EXPECT_EQ(m_eventReplay->getState(), PlaybackState::Stopped);
}

TEST_F(WindowsEventReplayTest, SeekToPosition)
{
    auto events = createTestEvents();
    size_t eventCount = events.size();

    bool loadResult = m_eventReplay->loadEvents(std::move(events));
    ASSERT_TRUE(loadResult);

    // Test seeking to valid positions
    for (size_t i = 0; i < eventCount; ++i)
    {
        bool seekResult = m_eventReplay->seekToPosition(i);
        EXPECT_TRUE(seekResult)
            << "Seeking to position " << i << " should succeed";
        EXPECT_EQ(m_eventReplay->getCurrentPosition(), i);
    }

    // Test seeking to invalid position
    bool seekResult = m_eventReplay->seekToPosition(eventCount + 10);
    EXPECT_FALSE(seekResult) << "Seeking beyond events should fail";
}

TEST_F(WindowsEventReplayTest, EventCallback)
{
    auto events = createTestEvents();

    bool eventCallbackInvoked = false;
    const Event* receivedEvent = nullptr;

    auto eventCallback = [&](const Event& event)
    {
        eventCallbackInvoked = true;
        receivedEvent = &event;
    };

    m_eventReplay->setEventCallback(eventCallback);

    bool loadResult = m_eventReplay->loadEvents(std::move(events));
    ASSERT_TRUE(loadResult);

    // Event callback functionality would be tested during actual playback
    // For now, just verify the callback can be set without errors
    SUCCEED();
}
#else
TEST_F(WindowsEventReplayTest, StartPlaybackFailsOnNonWindows)
{
    auto events = createTestEvents();

    // Load should fail on non-Windows
    bool loadResult = m_eventReplay->loadEvents(std::move(events));
    EXPECT_FALSE(loadResult);

    // Playback should also fail
    bool result = m_eventReplay->startPlayback();
    EXPECT_FALSE(result) << "Playback should fail on non-Windows platforms";
    EXPECT_EQ(m_eventReplay->getState(), PlaybackState::Stopped);
}
#endif

TEST_F(WindowsEventReplayTest, StopPlaybackWhenNotPlaying)
{
    // Should not crash when stopping playback that hasn't started
    EXPECT_EQ(m_eventReplay->getState(), PlaybackState::Stopped);
    m_eventReplay->stopPlayback();
    EXPECT_EQ(m_eventReplay->getState(), PlaybackState::Stopped);
}

// Test class teardown
TEST_F(WindowsEventReplayTest, DestructorStopsPlayback)
{
#ifdef _WIN32
    auto events = createTestEvents();
    if (m_eventReplay->loadEvents(std::move(events)))
    {
        m_eventReplay->startPlayback();
    }
#endif

    // Destructor should clean up properly
    m_eventReplay.reset();

    // If we get here without crashing, the test passes
    SUCCEED();
}
