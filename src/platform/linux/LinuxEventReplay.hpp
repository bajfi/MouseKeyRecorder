#pragma once

#include "core/IEventPlayer.hpp"
#include <X11/Xlib.h>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <set>

namespace MouseRecorder::Platform::Linux
{

/**
 * @brief Linux-specific implementation of event playback using X11
 *
 * This class replays mouse and keyboard events using X11 XTest extension
 * for synthetic event injection.
 */
class LinuxEventReplay : public Core::IEventPlayer
{
  public:
    LinuxEventReplay();
    ~LinuxEventReplay() override;

    // IEventPlayer interface
    bool loadEvents(std::vector<std::unique_ptr<Core::Event>> events) override;
    bool startPlayback(PlaybackCallback callback = nullptr) override;
    void pausePlayback() override;
    void resumePlayback() override;
    void stopPlayback() override;
    Core::PlaybackState getState() const noexcept override;
    void setPlaybackSpeed(double speed) override;
    double getPlaybackSpeed() const noexcept override;
    void setLoopPlayback(bool loop) override;
    bool isLoopEnabled() const noexcept override;
    size_t getCurrentPosition() const noexcept override;
    size_t getTotalEvents() const noexcept override;
    bool seekToPosition(size_t position) override;
    void setEventCallback(EventCallback callback) override;
    std::string getLastError() const override;

  private:
    /**
     * @brief Initialize X11 connection and XTest extension
     * @return true if initialization successful
     */
    bool initializeX11();

    /**
     * @brief Cleanup X11 resources
     */
    void cleanupX11();

    /**
     * @brief Main playback loop running in separate thread
     */
    void playbackLoop();

    /**
     * @brief Execute a single event
     * @param event Event to execute
     * @return true if event was executed successfully
     */
    bool executeEvent(const Core::Event& event);

    /**
     * @brief Execute mouse event
     * @param event Mouse event to execute
     * @return true if successful
     */
    bool executeMouseEvent(const Core::Event& event);

    /**
     * @brief Execute keyboard event
     * @param event Keyboard event to execute
     * @return true if successful
     */
    bool executeKeyboardEvent(const Core::Event& event);

    /**
     * @brief Convert key name to X11 keycode
     * @param keyName Human-readable key name
     * @return X11 keycode or 0 if not found
     */
    KeyCode getKeycodeFromName(const std::string& keyName);

    /**
     * @brief Calculate delay until next event
     * @param currentEventTime Current event timestamp
     * @param nextEventTime Next event timestamp
     * @return delay in milliseconds
     */
    std::chrono::milliseconds calculateDelay(
      uint64_t currentEventTime, uint64_t nextEventTime
    );

    /**
     * @brief Set playback state and notify callbacks
     * @param newState New playback state
     */
    void setState(Core::PlaybackState newState);

    /**
     * @brief Set last error message in thread-safe manner
     * @param error error message
     */
    void setLastError(const std::string& error);

  private:
    // X11 resources
    Display* m_display{nullptr};
    Window m_rootWindow{0};

    // Events and playback control
    std::vector<std::unique_ptr<Core::Event>> m_events;
    std::atomic<size_t> m_currentPosition{0};
    std::atomic<Core::PlaybackState> m_state{Core::PlaybackState::Stopped};

    // Threading and synchronization
    std::unique_ptr<std::thread> m_playbackThread;
    std::atomic<bool> m_shouldStop{false};
    std::mutex m_pauseMutex;
    std::condition_variable m_pauseCondition;
    std::atomic<bool> m_isPaused{false};

    // Configuration
    std::atomic<double> m_playbackSpeed{1.0};
    std::atomic<bool> m_loopEnabled{false};

    // Key state tracking for cleanup
    std::mutex m_pressedKeysMutex;
    std::set<KeyCode> m_pressedKeys;
    std::set<unsigned int> m_pressedButtons;

    // Callbacks
    PlaybackCallback m_playbackCallback;
    EventCallback m_eventCallback;

    // Error handling
    mutable std::mutex m_errorMutex;
    std::string m_lastError;
};

} // namespace MouseRecorder::Platform::Linux
