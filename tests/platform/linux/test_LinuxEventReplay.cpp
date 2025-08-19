#include <gtest/gtest.h>
#include "platform/linux/LinuxEventReplay.hpp"
#include "core/Event.hpp"
#include <thread>
#include <chrono>
#include <atomic>

using namespace MouseRecorder::Platform::Linux;
using namespace MouseRecorder::Core;

class LinuxEventReplayTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        m_eventPlayer = std::make_unique<LinuxEventReplay>();
    }

    void TearDown() override
    {
        if (m_eventPlayer &&
            m_eventPlayer->getState() != PlaybackState::Stopped)
        {
            m_eventPlayer->stopPlayback();
        }
        m_eventPlayer.reset();
    }

    std::vector<std::unique_ptr<Event>> createTestEvents()
    {
        std::vector<std::unique_ptr<Event>> events;

        // Create some test events
        events.push_back(EventFactory::createMouseMoveEvent({100, 100}));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        events.push_back(
            EventFactory::createMouseClickEvent({100, 100}, MouseButton::Left));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        events.push_back(EventFactory::createKeyPressEvent(65, "A"));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        events.push_back(EventFactory::createKeyReleaseEvent(65, "A"));

        return events;
    }

    std::unique_ptr<LinuxEventReplay> m_eventPlayer;
    std::atomic<bool> m_callbackCalled{false};
    std::atomic<PlaybackState> m_lastState{PlaybackState::Stopped};
};

TEST_F(LinuxEventReplayTest, InitialState)
{
    EXPECT_EQ(m_eventPlayer->getState(), PlaybackState::Stopped);
    EXPECT_EQ(m_eventPlayer->getCurrentPosition(), 0);
    EXPECT_EQ(m_eventPlayer->getTotalEvents(), 0);
    EXPECT_EQ(m_eventPlayer->getPlaybackSpeed(), 1.0);
    EXPECT_FALSE(m_eventPlayer->isLoopEnabled());
    EXPECT_TRUE(m_eventPlayer->getLastError().empty());
}

TEST_F(LinuxEventReplayTest, LoadEvents)
{
    auto events = createTestEvents();
    size_t eventCount = events.size();

    EXPECT_TRUE(m_eventPlayer->loadEvents(std::move(events)));
    EXPECT_EQ(m_eventPlayer->getTotalEvents(), eventCount);
    EXPECT_EQ(m_eventPlayer->getCurrentPosition(), 0);
}

TEST_F(LinuxEventReplayTest, LoadEventsWhilePlaying)
{
    auto events = createTestEvents();
    EXPECT_TRUE(m_eventPlayer->loadEvents(std::move(events)));

    // Start playback
    auto callback = [this](PlaybackState state, size_t current, size_t total)
    {
        m_callbackCalled.store(true);
        m_lastState.store(state);
        (void) current;
        (void) total;
    };

    bool started = m_eventPlayer->startPlayback(callback);
    if (started)
    {
        // Try to load events while playing - should fail
        auto moreEvents = createTestEvents();
        EXPECT_FALSE(m_eventPlayer->loadEvents(std::move(moreEvents)));
        EXPECT_FALSE(m_eventPlayer->getLastError().empty());

        m_eventPlayer->stopPlayback();
    }
}

TEST_F(LinuxEventReplayTest, StartPlaybackWithoutEvents)
{
    // Should fail when no events are loaded
    EXPECT_FALSE(m_eventPlayer->startPlayback());
    EXPECT_FALSE(m_eventPlayer->getLastError().empty());
    EXPECT_EQ(m_eventPlayer->getState(), PlaybackState::Stopped);
}

TEST_F(LinuxEventReplayTest, StartPlaybackWithEvents)
{
    auto events = createTestEvents();
    EXPECT_TRUE(m_eventPlayer->loadEvents(std::move(events)));

    auto callback = [this](PlaybackState state, size_t current, size_t total)
    {
        m_callbackCalled.store(true);
        m_lastState.store(state);
        (void) current;
        (void) total;
    };

    // Note: This test might fail in headless environments without X11
    bool started = m_eventPlayer->startPlayback(callback);

    if (started)
    {
        EXPECT_EQ(m_eventPlayer->getState(), PlaybackState::Playing);

        // Let it play for a short time
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // Should have called the callback
        EXPECT_TRUE(m_callbackCalled.load());

        m_eventPlayer->stopPlayback();
        EXPECT_EQ(m_eventPlayer->getState(), PlaybackState::Stopped);
    }
    else
    {
        // If we can't start playback (e.g., no X11), that's OK for this test
        EXPECT_FALSE(m_eventPlayer->getLastError().empty());
    }
}

TEST_F(LinuxEventReplayTest, PlaybackSpeed)
{
    // Test speed limits
    m_eventPlayer->setPlaybackSpeed(0.05); // Should be clamped to 0.1
    EXPECT_GE(m_eventPlayer->getPlaybackSpeed(), 0.1);

    m_eventPlayer->setPlaybackSpeed(15.0); // Should be clamped to 10.0
    EXPECT_LE(m_eventPlayer->getPlaybackSpeed(), 10.0);

    // Test normal speeds
    m_eventPlayer->setPlaybackSpeed(0.5);
    EXPECT_DOUBLE_EQ(m_eventPlayer->getPlaybackSpeed(), 0.5);

    m_eventPlayer->setPlaybackSpeed(2.0);
    EXPECT_DOUBLE_EQ(m_eventPlayer->getPlaybackSpeed(), 2.0);
}

TEST_F(LinuxEventReplayTest, LoopPlayback)
{
    EXPECT_FALSE(m_eventPlayer->isLoopEnabled());

    m_eventPlayer->setLoopPlayback(true);
    EXPECT_TRUE(m_eventPlayer->isLoopEnabled());

    m_eventPlayer->setLoopPlayback(false);
    EXPECT_FALSE(m_eventPlayer->isLoopEnabled());
}

TEST_F(LinuxEventReplayTest, LoopCount)
{
    // Test default loop count
    EXPECT_EQ(m_eventPlayer->getLoopCount(), 0);

    // Test setting loop count
    m_eventPlayer->setLoopCount(5);
    EXPECT_EQ(m_eventPlayer->getLoopCount(), 5);

    // Test infinite loop (0)
    m_eventPlayer->setLoopCount(0);
    EXPECT_EQ(m_eventPlayer->getLoopCount(), 0);

    // Test single loop (1)
    m_eventPlayer->setLoopCount(1);
    EXPECT_EQ(m_eventPlayer->getLoopCount(), 1);
}

TEST_F(LinuxEventReplayTest, SeekToPosition)
{
    auto events = createTestEvents();
    size_t eventCount = events.size();
    EXPECT_TRUE(m_eventPlayer->loadEvents(std::move(events)));

    // Test valid seek positions
    EXPECT_TRUE(m_eventPlayer->seekToPosition(0));
    EXPECT_EQ(m_eventPlayer->getCurrentPosition(), 0);

    EXPECT_TRUE(m_eventPlayer->seekToPosition(eventCount / 2));
    EXPECT_EQ(m_eventPlayer->getCurrentPosition(), eventCount / 2);

    // Test invalid seek position
    EXPECT_FALSE(m_eventPlayer->seekToPosition(eventCount + 1));
    EXPECT_FALSE(m_eventPlayer->getLastError().empty());
}

TEST_F(LinuxEventReplayTest, EventCallback)
{
    std::atomic<int> eventCallbackCount{0};

    auto eventCallback = [&eventCallbackCount](const Event& event)
    {
        eventCallbackCount++;
        (void) event;
    };

    m_eventPlayer->setEventCallback(eventCallback);

    auto events = createTestEvents();
    EXPECT_TRUE(m_eventPlayer->loadEvents(std::move(events)));

    bool started = m_eventPlayer->startPlayback();
    if (started)
    {
        // Let it play
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        m_eventPlayer->stopPlayback();

        // Should have called the event callback for some events
        // (might not be all events depending on timing)
        EXPECT_GT(eventCallbackCount.load(), 0);
    }
}
