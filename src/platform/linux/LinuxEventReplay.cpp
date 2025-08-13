#include "LinuxEventReplay.hpp"
#include "core/Event.hpp"
#include "application/MouseRecorderApp.hpp"
#include <X11/extensions/XTest.h>
#include <X11/XKBlib.h>
#include <spdlog/spdlog.h>
#include <algorithm>

// Undefine X11 macros that conflict with our enums
#ifdef KeyPress
#undef KeyPress
#endif
#ifdef KeyRelease
#undef KeyRelease
#endif

namespace MouseRecorder::Platform::Linux
{

LinuxEventReplay::LinuxEventReplay()
{
    spdlog::debug("LinuxEventReplay: Constructor");
}

LinuxEventReplay::~LinuxEventReplay()
{
    try
    {
        // Stop playback without logging if spdlog is shut down
        if (m_state.load() != Core::PlaybackState::Stopped)
        {
            m_shouldStop.store(true);
            m_state.store(Core::PlaybackState::Stopped);

            if (m_playbackThread && m_playbackThread->joinable())
            {
                m_playbackThread->join();
                m_playbackThread.reset();
            }
        }

        cleanupX11();
    }
    catch (...)
    {
        // Silently handle any exceptions during destruction
    }
}

bool LinuxEventReplay::loadEvents(
  std::vector<std::unique_ptr<Core::Event>> events
)
{
    spdlog::info("LinuxEventReplay: Loading {} events", events.size());

    if (m_state.load() != Core::PlaybackState::Stopped)
    {
        setLastError("Cannot load events while playback is active");
        return false;
    }

    m_events = std::move(events);
    m_currentPosition.store(0);

    spdlog::info(
      "LinuxEventReplay: {} events loaded successfully", m_events.size()
    );
    return true;
}

bool LinuxEventReplay::startPlayback(PlaybackCallback callback)
{
    spdlog::info("LinuxEventReplay: Starting playback");

    if (m_state.load() != Core::PlaybackState::Stopped)
    {
        setLastError("Playback is already active");
        return false;
    }

    if (m_events.empty())
    {
        setLastError("No events loaded for playback");
        return false;
    }

    if (!initializeX11())
    {
        return false;
    }

    m_playbackCallback = std::move(callback);
    m_shouldStop.store(false);
    m_isPaused.store(false);

    // Start playback thread
    m_playbackThread =
      std::make_unique<std::thread>(&LinuxEventReplay::playbackLoop, this);

    setState(Core::PlaybackState::Playing);
    spdlog::info("LinuxEventReplay: Playback started successfully");
    return true;
}

void LinuxEventReplay::pausePlayback()
{
    spdlog::info("LinuxEventReplay: Pausing playback");

    if (m_state.load() == Core::PlaybackState::Playing)
    {
        m_isPaused.store(true);
        setState(Core::PlaybackState::Paused);
    }
}

void LinuxEventReplay::resumePlayback()
{
    spdlog::info("LinuxEventReplay: Resuming playback");

    if (m_state.load() == Core::PlaybackState::Paused)
    {
        m_isPaused.store(false);
        m_pauseCondition.notify_all();
        setState(Core::PlaybackState::Playing);
    }
}

void LinuxEventReplay::stopPlayback()
{
    if (!Application::MouseRecorderApp::isLoggingShutdown())
    {
        spdlog::info("LinuxEventReplay: Stopping playback");
    }

    if (m_state.load() == Core::PlaybackState::Stopped)
    {
        return;
    }

    m_shouldStop.store(true);
    m_isPaused.store(false);
    m_pauseCondition.notify_all();

    if (m_playbackThread && m_playbackThread->joinable())
    {
        m_playbackThread->join();
        m_playbackThread.reset();
    }

    setState(Core::PlaybackState::Stopped);
    m_playbackCallback = nullptr;

    if (!Application::MouseRecorderApp::isLoggingShutdown())
    {
        spdlog::info("LinuxEventReplay: Playback stopped");
    }
}

Core::PlaybackState LinuxEventReplay::getState() const noexcept
{
    return m_state.load();
}

void LinuxEventReplay::setPlaybackSpeed(double speed)
{
    speed = std::max(0.1, std::min(10.0, speed)); // Clamp between 0.1x and 10x
    m_playbackSpeed.store(speed);
    spdlog::debug("LinuxEventReplay: Playback speed set to {:.2f}x", speed);
}

double LinuxEventReplay::getPlaybackSpeed() const noexcept
{
    return m_playbackSpeed.load();
}

void LinuxEventReplay::setLoopPlayback(bool loop)
{
    m_loopEnabled.store(loop);
    spdlog::debug("LinuxEventReplay: Loop playback set to {}", loop);
}

bool LinuxEventReplay::isLoopEnabled() const noexcept
{
    return m_loopEnabled.load();
}

size_t LinuxEventReplay::getCurrentPosition() const noexcept
{
    return m_currentPosition.load();
}

size_t LinuxEventReplay::getTotalEvents() const noexcept
{
    return m_events.size();
}

bool LinuxEventReplay::seekToPosition(size_t position)
{
    if (position >= m_events.size())
    {
        setLastError("Seek position out of range");
        return false;
    }

    m_currentPosition.store(position);
    spdlog::debug("LinuxEventReplay: Seeked to position {}", position);
    return true;
}

void LinuxEventReplay::setEventCallback(EventCallback callback)
{
    m_eventCallback = std::move(callback);
}

std::string LinuxEventReplay::getLastError() const
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    return m_lastError;
}

bool LinuxEventReplay::initializeX11()
{
    spdlog::debug("LinuxEventReplay: Initializing X11");

    m_display = XOpenDisplay(nullptr);
    if (!m_display)
    {
        setLastError("Failed to open X11 display");
        return false;
    }

    m_rootWindow = DefaultRootWindow(m_display);

    // Check for XTest extension
    int event_base, error_base, major, minor;
    if (!XTestQueryExtension(
          m_display, &event_base, &error_base, &major, &minor
        ))
    {
        setLastError("XTest extension not available");
        return false;
    }

    spdlog::debug(
      "LinuxEventReplay: X11 initialized successfully, XTest version {}.{}",
      major,
      minor
    );
    return true;
}

void LinuxEventReplay::cleanupX11()
{
    spdlog::debug("LinuxEventReplay: Cleaning up X11 resources");

    if (m_display)
    {
        XCloseDisplay(m_display);
        m_display = nullptr;
    }
}

void LinuxEventReplay::playbackLoop()
{
    spdlog::debug("LinuxEventReplay: Playback loop started");

    // Get first event time for potential future timing calculations
    [[maybe_unused]] auto startTime = std::chrono::steady_clock::now();
    [[maybe_unused]] uint64_t firstEventTime = 0;

    if (!m_events.empty())
    {
        firstEventTime = m_events[0]->getTimestampMs();
    }

    do
    {
        for (size_t i = m_currentPosition.load();
             i < m_events.size() && !m_shouldStop.load();
             ++i)
        {
            // Handle pause
            {
                std::unique_lock<std::mutex> lock(m_pauseMutex);
                m_pauseCondition.wait(
                  lock,
                  [this]
                  {
                      return !m_isPaused.load() || m_shouldStop.load();
                  }
                );
            }

            if (m_shouldStop.load())
            {
                break;
            }

            const auto& event = m_events[i];

            // Calculate delay
            if (i > 0)
            {
                uint64_t currentEventTime = m_events[i - 1]->getTimestampMs();
                uint64_t nextEventTime = event->getTimestampMs();
                auto delay = calculateDelay(currentEventTime, nextEventTime);

                if (delay.count() > 0)
                {
                    std::this_thread::sleep_for(delay);
                }
            }

            // Execute event callback
            if (m_eventCallback)
            {
                m_eventCallback(*event);
            }

            // Execute the event
            if (!executeEvent(*event))
            {
                spdlog::warn(
                  "LinuxEventReplay: Failed to execute event at position {}", i
                );
            }

            m_currentPosition.store(i + 1);

            // Update progress callback
            if (m_playbackCallback)
            {
                m_playbackCallback(m_state.load(), i + 1, m_events.size());
            }
        }

        // Handle looping
        if (m_loopEnabled.load() && !m_shouldStop.load())
        {
            m_currentPosition.store(0);
            spdlog::debug("LinuxEventReplay: Looping playback");
        }

    } while (m_loopEnabled.load() && !m_shouldStop.load());

    if (!m_shouldStop.load())
    {
        setState(Core::PlaybackState::Completed);
    }

    spdlog::debug("LinuxEventReplay: Playback loop ended");
}

bool LinuxEventReplay::executeEvent(const Core::Event& event)
{
    switch (event.getType())
    {
    case Core::EventType::MouseMove:
    case Core::EventType::MouseClick:
    case Core::EventType::MouseDoubleClick:
    case Core::EventType::MouseWheel:
        return executeMouseEvent(event);

    case Core::EventType::KeyPress:
    case Core::EventType::KeyRelease:
    case Core::EventType::KeyCombination:
        return executeKeyboardEvent(event);

    default:
        spdlog::warn("LinuxEventReplay: Unknown event type");
        return false;
    }
}

bool LinuxEventReplay::executeMouseEvent(const Core::Event& event)
{
    const auto* mouseData = event.getMouseData();
    if (!mouseData)
    {
        return false;
    }

    switch (event.getType())
    {
    case Core::EventType::MouseMove: {
        XTestFakeMotionEvent(
          m_display,
          DefaultScreen(m_display),
          mouseData->position.x,
          mouseData->position.y,
          0
        );
        break;
    }

    case Core::EventType::MouseClick: {
        // Move to position first
        XTestFakeMotionEvent(
          m_display,
          DefaultScreen(m_display),
          mouseData->position.x,
          mouseData->position.y,
          0
        );

        // Convert mouse button
        unsigned int button = 1;
        switch (mouseData->button)
        {
        case Core::MouseButton::Left:
            button = 1;
            break;
        case Core::MouseButton::Middle:
            button = 2;
            break;
        case Core::MouseButton::Right:
            button = 3;
            break;
        case Core::MouseButton::X1:
            button = 8;
            break;
        case Core::MouseButton::X2:
            button = 9;
            break;
        }

        // Press and release
        XTestFakeButtonEvent(m_display, button, True, 0);
        XTestFakeButtonEvent(m_display, button, False, 0);
        break;
    }

    case Core::EventType::MouseDoubleClick: {
        // Move to position first
        XTestFakeMotionEvent(
          m_display,
          DefaultScreen(m_display),
          mouseData->position.x,
          mouseData->position.y,
          0
        );

        // Convert mouse button
        unsigned int button = 1;
        switch (mouseData->button)
        {
        case Core::MouseButton::Left:
            button = 1;
            break;
        case Core::MouseButton::Middle:
            button = 2;
            break;
        case Core::MouseButton::Right:
            button = 3;
            break;
        case Core::MouseButton::X1:
            button = 8;
            break;
        case Core::MouseButton::X2:
            button = 9;
            break;
        }

        // Double click
        XTestFakeButtonEvent(m_display, button, True, 0);
        XTestFakeButtonEvent(m_display, button, False, 0);
        XTestFakeButtonEvent(m_display, button, True, 0);
        XTestFakeButtonEvent(m_display, button, False, 0);
        break;
    }

    case Core::EventType::MouseWheel: {
        // Move to position first
        XTestFakeMotionEvent(
          m_display,
          DefaultScreen(m_display),
          mouseData->position.x,
          mouseData->position.y,
          0
        );

        // Wheel direction
        unsigned int button = (mouseData->wheelDelta > 0) ? 4 : 5;
        int scrollCount =
          std::abs(mouseData->wheelDelta) / 120; // Standard wheel delta

        for (int i = 0; i < scrollCount; ++i)
        {
            XTestFakeButtonEvent(m_display, button, True, 0);
            XTestFakeButtonEvent(m_display, button, False, 0);
        }
        break;
    }

    default:
        return false;
    }

    XFlush(m_display);
    return true;
}

bool LinuxEventReplay::executeKeyboardEvent(const Core::Event& event)
{
    const auto* keyData = event.getKeyboardData();
    if (!keyData)
    {
        return false;
    }

    KeyCode keycode = getKeycodeFromName(keyData->keyName);
    if (keycode == 0)
    {
        spdlog::warn(
          "LinuxEventReplay: Could not find keycode for key '{}'",
          keyData->keyName
        );
        return false;
    }

    switch (event.getType())
    {
    case Core::EventType::KeyPress: {
        XTestFakeKeyEvent(m_display, keycode, True, 0);
        break;
    }

    case Core::EventType::KeyRelease: {
        XTestFakeKeyEvent(m_display, keycode, False, 0);
        break;
    }

    case Core::EventType::KeyCombination: {
        // For key combinations, we press all keys then release them
        // This is a simplified implementation
        XTestFakeKeyEvent(m_display, keycode, True, 0);
        XTestFakeKeyEvent(m_display, keycode, False, 0);
        break;
    }

    default:
        return false;
    }

    XFlush(m_display);
    return true;
}

KeyCode LinuxEventReplay::getKeycodeFromName(const std::string& keyName)
{
    if (!m_display)
    {
        return 0;
    }

    KeySym keysym = XStringToKeysym(keyName.c_str());
    if (keysym == NoSymbol)
    {
        return 0;
    }

    return XKeysymToKeycode(m_display, keysym);
}

std::chrono::milliseconds LinuxEventReplay::calculateDelay(
  uint64_t currentEventTime, uint64_t nextEventTime
)
{
    if (nextEventTime <= currentEventTime)
    {
        return std::chrono::milliseconds(0);
    }

    uint64_t originalDelay = nextEventTime - currentEventTime;
    double speed = m_playbackSpeed.load();

    if (speed <= 0.0)
    {
        speed = 1.0;
    }

    uint64_t adjustedDelay = static_cast<uint64_t>(originalDelay / speed);
    return std::chrono::milliseconds(adjustedDelay);
}

void LinuxEventReplay::setState(Core::PlaybackState newState)
{
    m_state.store(newState);

    if (m_playbackCallback)
    {
        m_playbackCallback(newState, m_currentPosition.load(), m_events.size());
    }
}

void LinuxEventReplay::setLastError(const std::string& error)
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    m_lastError = error;
    spdlog::error("LinuxEventReplay: {}", error);
}

} // namespace MouseRecorder::Platform::Linux
