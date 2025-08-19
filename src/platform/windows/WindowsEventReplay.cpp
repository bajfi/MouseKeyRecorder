#include "WindowsEventReplay.hpp"

namespace MouseRecorder::Platform::Windows
{

WindowsEventReplay::WindowsEventReplay()
    : m_state(Core::PlaybackState::Stopped),
      m_playbackSpeed(1.0),
      m_loopPlayback(false),
      m_loopCount(1),
      m_currentPosition(0),
      m_shouldStop(false)
{}

WindowsEventReplay::~WindowsEventReplay()
{
    stopPlayback();
}

bool WindowsEventReplay::loadEvents(
    std::vector<std::unique_ptr<Core::Event>> events)
{
    // Stub: Only implemented on Windows
    setLastError("Not implemented on non-Windows platforms");
    return false;
}

bool WindowsEventReplay::startPlayback(PlaybackCallback callback)
{
    // Stub
    return false;
}

void WindowsEventReplay::stopPlayback()
{
    // Stub
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
    // Stub
    return false;
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
}

} // namespace MouseRecorder::Platform::Windows
