// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "LinuxEventReplay.hpp"
#include "core/Event.hpp"
#include "application/MouseRecorderApp.hpp"
#include <X11/extensions/XTest.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include "core/SpdlogConfig.hpp"
#include <algorithm>
#include <csignal>
#include <atomic>
#include <string>

// Undefine X11 macros that conflict with our enums
#ifdef KeyPress
#undef KeyPress
#endif
#ifdef KeyRelease
#undef KeyRelease
#endif

namespace MouseRecorder::Platform::Linux
{

// Global cleanup for emergency X11 situations
static std::atomic<Display*> g_emergency_display{nullptr};

static void emergencyCleanupHandler(int signal)
{
    Display* display = g_emergency_display.load();
    if (display)
    {
        XCloseDisplay(display);
        g_emergency_display.store(nullptr);
    }
    std::exit(signal);
}

LinuxEventReplay::LinuxEventReplay()
{
    spdlog::debug("LinuxEventReplay: Constructor");
    // Install signal handlers for emergency cleanup
    std::signal(SIGSEGV, emergencyCleanupHandler);
    std::signal(SIGTERM, emergencyCleanupHandler);
    std::signal(SIGINT, emergencyCleanupHandler);
}

LinuxEventReplay::~LinuxEventReplay()
{
    spdlog::debug("LinuxEventReplay: Destructor called");

    // Stop playback and clean up resources immediately
    if (m_state.load() != Core::PlaybackState::Stopped)
    {
        m_shouldStop.store(true);
        m_state.store(Core::PlaybackState::Stopped);

        if (m_playbackThread && m_playbackThread->joinable())
        {
            try
            {
                // Longer timeout for thread cleanup in destructor
                if (m_playbackThread->joinable())
                {
                    m_playbackThread->join();
                }
                m_playbackThread.reset();
            }
            catch (const std::exception& e)
            {
                spdlog::error(
                    "LinuxEventReplay: Exception in destructor thread cleanup: "
                    "{}",
                    e.what());
            }
        }
    }

    cleanupX11();

    // Clear emergency display reference
    if (g_emergency_display.load() == m_display)
    {
        g_emergency_display.store(nullptr);
    }

    spdlog::debug("LinuxEventReplay: Destructor completed");
}

bool LinuxEventReplay::loadEvents(
    std::vector<std::unique_ptr<Core::Event>> events)
{
    spdlog::info("LinuxEventReplay: Loading {} events", events.size());

    // Enhanced state checking with timeout for safety
    auto currentState = m_state.load();
    if (currentState != Core::PlaybackState::Stopped &&
        currentState != Core::PlaybackState::Completed &&
        currentState != Core::PlaybackState::Error)
    {
        spdlog::error(
            "LinuxEventReplay: Cannot load events while playback is active "
            "(state: {})",
            static_cast<int>(currentState));
        setLastError("Cannot load events while playback is active");
        return false;
    }

    // Extra safety: wait for any potential thread cleanup
    if (m_playbackThread && m_playbackThread->joinable())
    {
        spdlog::warn("LinuxEventReplay: Waiting for thread cleanup during "
                     "event loading");
        m_playbackThread->join();
        m_playbackThread.reset();
    }

    m_events = std::move(events);
    m_currentPosition.store(0);
    m_totalEvents.store(m_events.size()); // Store thread-safe total count

    // Reset state to Stopped when loading new events
    setState(Core::PlaybackState::Stopped);

    // Clear any tracked pressed keys
    {
        std::lock_guard<std::mutex> lock(m_pressedKeysMutex);
        m_pressedKeys.clear();
        m_pressedButtons.clear();
    }

    spdlog::info("LinuxEventReplay: {} events loaded successfully",
                 m_events.size());
    return true;
}

bool LinuxEventReplay::startPlayback(PlaybackCallback callback)
{
    spdlog::info("LinuxEventReplay: Starting playback");

    auto currentState = m_state.load();
    if (currentState != Core::PlaybackState::Stopped &&
        currentState != Core::PlaybackState::Completed)
    {
        spdlog::error(
            "LinuxEventReplay: Playback is already active (state: {})",
            static_cast<int>(currentState));
        setLastError("Playback is already active");
        return false;
    }

    if (m_events.empty())
    {
        setLastError("No events loaded for playback");
        return false;
    }

    // Ensure any previous thread is fully cleaned up
    if (m_playbackThread)
    {
        if (m_playbackThread->joinable())
        {
            spdlog::warn("LinuxEventReplay: Joining previous playback thread");
            m_playbackThread->join();
        }
        m_playbackThread.reset();
    }

    if (!initializeX11())
    {
        return false;
    }

    m_playbackCallback = std::move(callback);
    m_shouldStop.store(false);

    // Reset position when starting playback (especially important for replay)
    m_currentPosition.store(0);
    // Reset loop iteration counter
    m_currentLoopIteration.store(0);

    // Start playback thread
    try
    {
        m_playbackThread = std::make_unique<std::thread>(
            &LinuxEventReplay::playbackLoop, this);
    }
    catch (const std::exception& e)
    {
        spdlog::error("LinuxEventReplay: Failed to create playback thread: {}",
                      e.what());
        setLastError("Failed to create playback thread");
        cleanupX11();
        return false;
    }

    setState(Core::PlaybackState::Playing);
    spdlog::info("LinuxEventReplay: Playback started successfully");
    return true;
}

void LinuxEventReplay::stopPlayback()
{
    spdlog::info("LinuxEventReplay: Stopping playback");

    auto currentState = m_state.load();
    if (currentState == Core::PlaybackState::Stopped)
    {
        return;
    }

    // Signal the playback thread to stop
    m_shouldStop.store(true);

    // Set state to stopping to prevent race conditions
    setState(Core::PlaybackState::Stopped);

    if (m_playbackThread && m_playbackThread->joinable())
    {
        // Give the thread some time to cleanup gracefully
        try
        {
            m_playbackThread->join();
        }
        catch (const std::exception& e)
        {
            spdlog::error("LinuxEventReplay: Exception during thread join: {}",
                          e.what());
        }
        m_playbackThread.reset();
    }

    // Cleanup input state to prevent keyboard/mouse state corruption
    cleanupInputState();

    // Clear tracked keys
    {
        std::lock_guard<std::mutex> lock(m_pressedKeysMutex);
        m_pressedKeys.clear();
        m_pressedButtons.clear();
    }

    m_playbackCallback = nullptr;

    spdlog::info("LinuxEventReplay: Playback stopped");
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

void LinuxEventReplay::setLoopCount(int count)
{
    m_loopCount.store(count);
    spdlog::debug("LinuxEventReplay: Loop count set to {}", count);
}

int LinuxEventReplay::getLoopCount() const noexcept
{
    return m_loopCount.load();
}

size_t LinuxEventReplay::getCurrentPosition() const noexcept
{
    return m_currentPosition.load();
}

size_t LinuxEventReplay::getTotalEvents() const noexcept
{
    return m_totalEvents.load();
}

bool LinuxEventReplay::seekToPosition(size_t position)
{
    if (position >= m_totalEvents.load())
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

    // Cleanup any existing connection first
    if (m_display)
    {
        XCloseDisplay(m_display);
        m_display = nullptr;
    }

    m_display = XOpenDisplay(nullptr);
    if (!m_display)
    {
        setLastError("Failed to open X11 display");
        return false;
    }

    // Set emergency display for signal handler
    g_emergency_display.store(m_display);

    m_rootWindow = DefaultRootWindow(m_display);

    // Check for XTest extension
    int event_base, error_base, major, minor;
    if (!XTestQueryExtension(
            m_display, &event_base, &error_base, &major, &minor))
    {
        setLastError("XTest extension not available");
        cleanupX11();
        return false;
    }

    // Enable synchronous mode for more predictable behavior during debugging
    XSynchronize(m_display, True);

    spdlog::debug(
        "LinuxEventReplay: X11 initialized successfully, XTest version {}.{}",
        major,
        minor);
    return true;
}

void LinuxEventReplay::cleanupInputState()
{
    spdlog::debug("LinuxEventReplay: Cleaning up input state");

    if (!m_display)
    {
        return;
    }

    try
    {
        // Force synchronous cleanup to ensure all pending events are processed
        XSync(m_display, True);

        // Release all tracked pressed keys first
        {
            std::lock_guard<std::mutex> lock(m_pressedKeysMutex);
            for (KeyCode keycode : m_pressedKeys)
            {
                XTestFakeKeyEvent(m_display, keycode, False, 0);
                spdlog::debug("LinuxEventReplay: Released tracked key {}",
                              keycode);
            }
            m_pressedKeys.clear();

            for (unsigned int button : m_pressedButtons)
            {
                XTestFakeButtonEvent(m_display, button, False, 0);
                spdlog::debug("LinuxEventReplay: Released tracked button {}",
                              button);
            }
            m_pressedButtons.clear();
        }

        // Reset any potentially stuck keys/buttons - this is critical for
        // preventing input corruption
        // Release all possible mouse buttons (1-9) as additional safety
        for (unsigned int button = 1; button <= 9; ++button)
        {
            XTestFakeButtonEvent(m_display, button, False, 0);
        }

        // Release common modifier keys that might be stuck
        const std::vector<KeySym> modifierKeys = {XK_Shift_L,
                                                  XK_Shift_R,
                                                  XK_Control_L,
                                                  XK_Control_R,
                                                  XK_Alt_L,
                                                  XK_Alt_R,
                                                  XK_Meta_L,
                                                  XK_Meta_R,
                                                  XK_Super_L,
                                                  XK_Super_R,
                                                  XK_space,
                                                  XK_Return,
                                                  XK_Tab,
                                                  XK_Escape};

        for (KeySym keysym : modifierKeys)
        {
            KeyCode keycode = XKeysymToKeycode(m_display, keysym);
            if (keycode != 0)
            {
                XTestFakeKeyEvent(m_display, keycode, False, 0);
            }
        }

        // Ensure all events are flushed and processed immediately
        XFlush(m_display);
        XSync(m_display, False);

        spdlog::debug("LinuxEventReplay: Input state cleanup completed");
    }
    catch (const std::exception& e)
    {
        spdlog::error(
            "LinuxEventReplay: Exception during input state cleanup: {}",
            e.what());
    }
    catch (...)
    {
        spdlog::warn(
            "LinuxEventReplay: Unknown exception during input state cleanup");
    }
}

void LinuxEventReplay::cleanupX11()
{
    spdlog::debug("LinuxEventReplay: Cleaning up X11 resources");

    if (m_display)
    {
        // Clear emergency reference first
        if (g_emergency_display.load() == m_display)
        {
            g_emergency_display.store(nullptr);
        }

        // Clean up input state first
        cleanupInputState();

        try
        {
            XCloseDisplay(m_display);
        }
        catch (...)
        {
            spdlog::error(
                "LinuxEventReplay: Exception while closing X11 display");
        }

        m_display = nullptr;
        m_rootWindow = 0;
    }
}

void LinuxEventReplay::playbackLoop()
{
    spdlog::debug("LinuxEventReplay: Playback loop started");

    try
    {
        // Get first event time for potential future timing calculations
        [[maybe_unused]] auto startTime = std::chrono::steady_clock::now();
        [[maybe_unused]] uint64_t firstEventTime = 0;

        if (!m_events.empty())
        {
            firstEventTime = m_events[0]->getTimestampMs();
        }

        // Reset loop iteration counter
        m_currentLoopIteration.store(0);

        do
        {
            // Increment loop iteration for finite loops
            if (m_loopEnabled.load() && m_loopCount.load() > 0)
            {
                m_currentLoopIteration.fetch_add(1);
                spdlog::debug("LinuxEventReplay: Starting loop iteration {}/{}",
                              m_currentLoopIteration.load(),
                              m_loopCount.load());
            }
            for (size_t i = m_currentPosition.load();
                 i < m_events.size() && !m_shouldStop.load();
                 ++i)
            {
                // Safety check: ensure we're not accessing invalid memory
                if (i >= m_events.size())
                {
                    spdlog::error(
                        "LinuxEventReplay: Index {} out of bounds (size: {})",
                        i,
                        m_events.size());
                    break;
                }

                if (m_shouldStop.load())
                {
                    break;
                }

                // Additional safety check after potential wait
                if (i >= m_events.size())
                {
                    spdlog::warn(
                        "LinuxEventReplay: Event index invalid after wait");
                    break;
                }

                const auto& event = m_events[i];
                if (!event)
                {
                    spdlog::error("LinuxEventReplay: Null event at position {}",
                                  i);
                    continue;
                }

                // Calculate delay
                if (i > 0 && i - 1 < m_events.size())
                {
                    uint64_t currentEventTime =
                        m_events[i - 1]->getTimestampMs();
                    uint64_t nextEventTime = event->getTimestampMs();
                    auto delay =
                        calculateDelay(currentEventTime, nextEventTime);

                    if (delay.count() > 0)
                    {
                        std::this_thread::sleep_for(delay);
                    }
                }

                // Execute event callback
                if (m_eventCallback)
                {
                    try
                    {
                        m_eventCallback(*event);
                    }
                    catch (const std::exception& e)
                    {
                        spdlog::error(
                            "LinuxEventReplay: Exception in event callback: {}",
                            e.what());
                    }
                }

                // Execute the event with error handling
                try
                {
                    if (!executeEvent(*event))
                    {
                        spdlog::warn(
                            "LinuxEventReplay: Failed to execute event at "
                            "position {}",
                            i);
                        // Continue with next event rather than stopping
                    }
                }
                catch (const std::exception& e)
                {
                    spdlog::error(
                        "LinuxEventReplay: Exception executing event {}: {}",
                        i,
                        e.what());
                    // Continue with next event
                }

                m_currentPosition.store(i + 1);

                // Update progress callback
                if (m_playbackCallback)
                {
                    try
                    {
                        m_playbackCallback(
                            m_state.load(), i + 1, m_totalEvents.load());
                    }
                    catch (const std::exception& e)
                    {
                        spdlog::error(
                            "LinuxEventReplay: Exception in playback callback: "
                            "{}",
                            e.what());
                    }
                }
            }

            // Handle looping
            if (m_loopEnabled.load() && !m_shouldStop.load())
            {
                int loopCount = m_loopCount.load();
                int currentIteration = m_currentLoopIteration.load();

                // Check if we should continue looping
                bool shouldContinueLoop = false;
                if (loopCount == 0) // Infinite loop
                {
                    shouldContinueLoop = true;
                    spdlog::debug("LinuxEventReplay: Continuing infinite loop");
                }
                else if (currentIteration < loopCount) // Finite loop
                {
                    shouldContinueLoop = true;
                    spdlog::debug(
                        "LinuxEventReplay: Continuing loop iteration {}/{}",
                        currentIteration,
                        loopCount);
                }
                else
                {
                    spdlog::debug(
                        "LinuxEventReplay: Completed {} loops, stopping",
                        loopCount);
                }

                if (shouldContinueLoop)
                {
                    m_currentPosition.store(0);
                }
                else
                {
                    // Exit the loop for finite loops that have completed
                    break;
                }
            }

        } while (m_loopEnabled.load() && !m_shouldStop.load());

        if (!m_shouldStop.load())
        {
            // Clean up input state immediately when playback completes normally
            cleanupInputState();
            setState(Core::PlaybackState::Completed);
        }
        else
        {
            // If stopped explicitly, ensure state is set to Stopped
            cleanupInputState();
            setState(Core::PlaybackState::Stopped);
        }
    }
    catch (const std::exception& e)
    {
        spdlog::error("LinuxEventReplay: Exception in playback loop: {}",
                      e.what());
        setLastError("Playback loop encountered an error: " +
                     std::string(e.what()));
        setState(Core::PlaybackState::Error);
    }
    catch (...)
    {
        spdlog::error("LinuxEventReplay: Unknown exception in playback loop");
        setLastError("Unknown error occurred during playback");
        setState(Core::PlaybackState::Error);
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
        XTestFakeMotionEvent(m_display,
                             DefaultScreen(m_display),
                             mouseData->position.x,
                             mouseData->position.y,
                             0);
        // Ensure mouse movement is immediately processed
        XFlush(m_display);
        XSync(m_display, False);
        break;
    }

    case Core::EventType::MouseClick: {
        // Move to position first
        XTestFakeMotionEvent(m_display,
                             DefaultScreen(m_display),
                             mouseData->position.x,
                             mouseData->position.y,
                             0);

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
        // Ensure click events are immediately processed
        XFlush(m_display);
        XSync(m_display, False);
        break;
    }

    case Core::EventType::MouseDoubleClick: {
        // Move to position first
        XTestFakeMotionEvent(m_display,
                             DefaultScreen(m_display),
                             mouseData->position.x,
                             mouseData->position.y,
                             0);

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
        // Ensure double-click events are immediately processed
        XFlush(m_display);
        XSync(m_display, False);
        break;
    }

    case Core::EventType::MouseWheel: {
        // Move to position first
        XTestFakeMotionEvent(m_display,
                             DefaultScreen(m_display),
                             mouseData->position.x,
                             mouseData->position.y,
                             0);

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
        spdlog::warn("LinuxEventReplay: Could not find keycode for key '{}'",
                     keyData->keyName);
        return false;
    }

    switch (event.getType())
    {
    case Core::EventType::KeyPress: {
        XTestFakeKeyEvent(m_display, keycode, True, 0);
        // Track pressed key for cleanup
        {
            std::lock_guard<std::mutex> lock(m_pressedKeysMutex);
            m_pressedKeys.insert(keycode);
        }
        // Ensure key press is immediately processed
        XFlush(m_display);
        XSync(m_display, False);
        break;
    }

    case Core::EventType::KeyRelease: {
        XTestFakeKeyEvent(m_display, keycode, False, 0);
        // Remove from pressed keys tracking
        {
            std::lock_guard<std::mutex> lock(m_pressedKeysMutex);
            m_pressedKeys.erase(keycode);
        }
        // Ensure key release is immediately processed
        XFlush(m_display);
        XSync(m_display, False);
        break;
    }

    case Core::EventType::KeyCombination: {
        // For key combinations, we press all keys then release them
        // This is a simplified implementation
        XTestFakeKeyEvent(m_display, keycode, True, 0);
        XTestFakeKeyEvent(m_display, keycode, False, 0);
        // No need to track since we immediately release
        // Ensure key combination is immediately processed
        XFlush(m_display);
        XSync(m_display, False);
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
    uint64_t currentEventTime, uint64_t nextEventTime)
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
        m_playbackCallback(
            newState, m_currentPosition.load(), m_totalEvents.load());
    }
}

void LinuxEventReplay::setLastError(const std::string& error)
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    m_lastError = error;
    spdlog::error("LinuxEventReplay: {}", error);
}

} // namespace MouseRecorder::Platform::Linux
