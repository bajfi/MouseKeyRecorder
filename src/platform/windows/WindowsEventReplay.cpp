#include "WindowsEventReplay.hpp"
#include "core/Event.hpp"
#include "core/SpdlogConfig.hpp"
#include <chrono>
#include <thread>

namespace MouseRecorder::Platform::Windows
{

WindowsEventReplay::WindowsEventReplay()
    : m_state(Core::PlaybackState::Stopped),
      m_playbackSpeed(1.0),
      m_loopPlayback(false),
      m_loopCount(1),
      m_currentPosition(0),
      m_shouldStop(false)
{
    m_lastEventTime = std::chrono::steady_clock::now();
}

WindowsEventReplay::~WindowsEventReplay()
{
    stopPlayback();
}

bool WindowsEventReplay::loadEvents(
    std::vector<std::unique_ptr<Core::Event>> events)
{
    if (m_state.load() == Core::PlaybackState::Playing)
    {
        setLastError("Cannot load events while playback is active");
        return false;
    }

    try
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_events = std::move(events);
        m_currentPosition.store(0);
        m_state.store(Core::PlaybackState::Stopped);

        spdlog::info("WindowsEventReplay: Loaded {} events for playback",
                     m_events.size());
        return true;
    }
    catch (const std::exception& e)
    {
        setLastError("Failed to load events: " + std::string(e.what()));
        return false;
    }
}

bool WindowsEventReplay::startPlayback(PlaybackCallback callback)
{
    if (m_state.load() == Core::PlaybackState::Playing)
    {
        setLastError("Playback is already active");
        return false;
    }

    if (m_events.empty())
    {
        setLastError("No events loaded for playback");
        return false;
    }

    try
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_playbackCallback = callback;
        m_shouldStop.store(false);
        m_state.store(Core::PlaybackState::Playing);

        // Start playback thread
        m_playbackThread = std::make_unique<std::thread>(
            &WindowsEventReplay::playbackThreadFunc, this);

        spdlog::info("WindowsEventReplay: Starting playback of {} events",
                     m_events.size());
        return true;
    }
    catch (const std::exception& e)
    {
        setLastError("Failed to start playback: " + std::string(e.what()));
        m_state.store(Core::PlaybackState::Error);
        return false;
    }
}

void WindowsEventReplay::stopPlayback()
{
    if (m_state.load() == Core::PlaybackState::Stopped)
    {
        return;
    }

    spdlog::info("WindowsEventReplay: Stopping playback");

    m_shouldStop.store(true);
    m_cv.notify_all();

    if (m_playbackThread && m_playbackThread->joinable())
    {
        m_playbackThread->join();
        m_playbackThread.reset();
    }

    m_state.store(Core::PlaybackState::Stopped);
    spdlog::info("WindowsEventReplay: Playback stopped");
}

Core::PlaybackState WindowsEventReplay::getState() const noexcept
{
    return m_state;
}

double WindowsEventReplay::getPlaybackSpeed() const noexcept
{
    return m_playbackSpeed;
}

void WindowsEventReplay::setPlaybackSpeed(double speed)
{
    m_playbackSpeed = speed;
}

void WindowsEventReplay::setLoopPlayback(bool loop)
{
    m_loopPlayback = loop;
}

bool WindowsEventReplay::isLoopEnabled() const noexcept
{
    return m_loopPlayback;
}

void WindowsEventReplay::setLoopCount(int count)
{
    m_loopCount = count;
}

int WindowsEventReplay::getLoopCount() const noexcept
{
    return m_loopCount;
}

size_t WindowsEventReplay::getCurrentPosition() const noexcept
{
    return m_currentPosition;
}

size_t WindowsEventReplay::getTotalEvents() const noexcept
{
    return m_events.size();
}

bool WindowsEventReplay::seekToPosition(size_t position)
{
    if (m_state.load() == Core::PlaybackState::Playing)
    {
        setLastError("Cannot seek while playback is active");
        return false;
    }

    if (position >= m_events.size())
    {
        setLastError("Seek position out of range");
        return false;
    }

    m_currentPosition.store(position);
    return true;
}

void WindowsEventReplay::setEventCallback(EventCallback callback)
{
    m_eventCallback = callback;
}

std::string WindowsEventReplay::getLastError() const
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    return m_lastError;
}

void WindowsEventReplay::setLastError(const std::string& error)
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    m_lastError = error;
    spdlog::error("WindowsEventReplay: {}", error);
}

void WindowsEventReplay::playbackThreadFunc()
{
    try
    {
        spdlog::debug("WindowsEventReplay: Playback thread started");

        int currentLoop = 0;
        bool shouldContinue = true;

        while (shouldContinue && !m_shouldStop.load())
        {
            // Play through all events in this loop
            for (size_t i = m_currentPosition.load();
                 i < m_events.size() && !m_shouldStop.load();
                 ++i)
            {
                const auto& event = m_events[i];

                // Calculate timing delay
                if (i > 0 && m_playbackSpeed.load() > 0.0)
                {
                    auto currentTime = event->getTimestamp();
                    auto previousTime = m_events[i - 1]->getTimestamp();
                    auto delay = currentTime - previousTime;

                    // Apply speed scaling
                    auto scaledDelay =
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            delay / m_playbackSpeed.load());

                    if (scaledDelay.count() > 0)
                    {
                        std::this_thread::sleep_for(scaledDelay);
                    }
                }

                // Inject the event
                if (!injectEvent(*event))
                {
                    spdlog::warn("WindowsEventReplay: Failed to inject event "
                                 "at position {}",
                                 i);
                }

                // Update position
                m_currentPosition.store(i);

                // Call progress callback
                if (m_playbackCallback)
                {
                    m_playbackCallback(m_state.load(), i, m_events.size());
                }

                // Call event callback
                if (m_eventCallback)
                {
                    m_eventCallback(*event);
                }
            }

            currentLoop++;

            // Check if we should continue looping
            if (m_loopPlayback.load())
            {
                int maxLoops = m_loopCount.load();
                if (maxLoops > 0 && currentLoop >= maxLoops)
                {
                    shouldContinue = false;
                }
                else
                {
                    m_currentPosition.store(0); // Reset for next loop
                }
            }
            else
            {
                shouldContinue = false;
            }
        }

        if (m_shouldStop.load())
        {
            m_state.store(Core::PlaybackState::Stopped);
        }
        else
        {
            m_state.store(Core::PlaybackState::Completed);
        }

        // Final callback
        if (m_playbackCallback)
        {
            m_playbackCallback(
                m_state.load(), m_currentPosition.load(), m_events.size());
        }

        spdlog::debug("WindowsEventReplay: Playback thread completed");
    }
    catch (const std::exception& e)
    {
        setLastError("Playback thread error: " + std::string(e.what()));
        m_state.store(Core::PlaybackState::Error);
    }
}

bool WindowsEventReplay::injectEvent(const Core::Event& event)
{
    try
    {
        INPUT input = {};
        bool result = false;

        switch (event.getType())
        {
        case Core::EventType::MouseMove: {
            input.type = INPUT_MOUSE;
            input.mi.dx = event.getPosition().x;
            input.mi.dy = event.getPosition().y;
            input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

            // Convert to screen coordinates (0-65535 range)
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            input.mi.dx = (input.mi.dx * 65535) / screenWidth;
            input.mi.dy = (input.mi.dy * 65535) / screenHeight;

            result = SendInput(1, &input, sizeof(INPUT)) == 1;
            break;
        }

        case Core::EventType::MouseClick: {
            input.type = INPUT_MOUSE;
            input.mi.dx = event.getPosition().x;
            input.mi.dy = event.getPosition().y;

            // Convert to screen coordinates
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            input.mi.dx = (input.mi.dx * 65535) / screenWidth;
            input.mi.dy = (input.mi.dy * 65535) / screenHeight;

            input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE;

            switch (event.getMouseButton())
            {
            case Core::MouseButton::Left:
                input.mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
                break;
            case Core::MouseButton::Right:
                input.mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
                break;
            case Core::MouseButton::Middle:
                input.mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
                break;
            default:
                return false;
            }

            result = SendInput(1, &input, sizeof(INPUT)) == 1;

            // Send corresponding button up event
            input.mi.dwFlags &= ~(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_RIGHTDOWN |
                                  MOUSEEVENTF_MIDDLEDOWN);
            switch (event.getMouseButton())
            {
            case Core::MouseButton::Left:
                input.mi.dwFlags |= MOUSEEVENTF_LEFTUP;
                break;
            case Core::MouseButton::Right:
                input.mi.dwFlags |= MOUSEEVENTF_RIGHTUP;
                break;
            case Core::MouseButton::Middle:
                input.mi.dwFlags |= MOUSEEVENTF_MIDDLEUP;
                break;
            }

            result = result && (SendInput(1, &input, sizeof(INPUT)) == 1);
            break;
        }

        case Core::EventType::MouseWheel: {
            input.type = INPUT_MOUSE;
            input.mi.dx = event.getPosition().x;
            input.mi.dy = event.getPosition().y;
            input.mi.mouseData = event.getWheelDelta();
            input.mi.dwFlags = MOUSEEVENTF_WHEEL | MOUSEEVENTF_ABSOLUTE;

            // Convert to screen coordinates
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            input.mi.dx = (input.mi.dx * 65535) / screenWidth;
            input.mi.dy = (input.mi.dy * 65535) / screenHeight;

            result = SendInput(1, &input, sizeof(INPUT)) == 1;
            break;
        }

        case Core::EventType::KeyPress:
        case Core::EventType::KeyRelease: {
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = static_cast<WORD>(event.getKeyCode());
            input.ki.dwFlags = (event.getType() == Core::EventType::KeyRelease)
                                   ? KEYEVENTF_KEYUP
                                   : 0;

            result = SendInput(1, &input, sizeof(INPUT)) == 1;
            break;
        }

        default:
            spdlog::warn(
                "WindowsEventReplay: Unsupported event type for injection");
            return false;
        }

        if (!result)
        {
            DWORD error = GetLastError();
            spdlog::warn("WindowsEventReplay: SendInput failed with error: {}",
                         error);
        }

        return result;
    }
    catch (const std::exception& e)
    {
        spdlog::error("WindowsEventReplay: Error injecting event: {}",
                      e.what());
        return false;
    }
}

} // namespace MouseRecorder::Platform::Windows
