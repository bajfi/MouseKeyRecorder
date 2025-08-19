#pragma once

#include "core/IEventPlayer.hpp"
#include <windows.h>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <condition_variable>

namespace MouseRecorder::Platform::Windows
{

/**
 * @brief Windows-specific implementation of event playback using Win32 API
 *
 * This class replays mouse and keyboard events using Windows SendInput API.
 */
class WindowsEventReplay : public Core::IEventPlayer
{
  public:
    WindowsEventReplay();
    ~WindowsEventReplay() override;

    // IEventPlayer interface
    bool loadEvents(std::vector<std::unique_ptr<Core::Event>> events) override;
    bool startPlayback(PlaybackCallback callback = nullptr) override;
    void stopPlayback() override;
    Core::PlaybackState getState() const noexcept override;
    void setPlaybackSpeed(double speed) override;
    double getPlaybackSpeed() const noexcept override;
    void setLoopPlayback(bool loop) override;
    bool isLoopEnabled() const noexcept override;
    void setLoopCount(int count) override;
    int getLoopCount() const noexcept override;
    size_t getCurrentPosition() const noexcept override;
    size_t getTotalEvents() const noexcept override;
    bool seekToPosition(size_t position) override;
    void setEventCallback(EventCallback callback) override;
    std::string getLastError() const override;

  private:
    /**
     * @brief Playback thread function
     */
    void playbackThreadFunc();

    /**
     * @brief Inject a single event using SendInput
     */
    bool injectEvent(const Core::Event& event);

    // State
    std::atomic<Core::PlaybackState> m_state;
    std::atomic<double> m_playbackSpeed;
    std::atomic<bool> m_loopPlayback;
    std::atomic<int> m_loopCount;
    std::atomic<size_t> m_currentPosition;
    std::vector<std::unique_ptr<Core::Event>> m_events;

    // Threading
    std::unique_ptr<std::thread> m_playbackThread;
    std::atomic<bool> m_shouldStop;
    std::mutex m_mutex;
    std::condition_variable m_cv;

    // Callbacks
    PlaybackCallback m_playbackCallback;
    EventCallback m_eventCallback;

    // Error handling
    mutable std::mutex m_errorMutex;
    std::string m_lastError;

    // Timing
    std::chrono::steady_clock::time_point m_lastEventTime;
};

} // namespace MouseRecorder::Platform::Windows
