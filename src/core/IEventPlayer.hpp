#pragma once

#include "Event.hpp"
#include <vector>
#include <memory>
#include <functional>

namespace MouseRecorder::Core
{

/**
 * @brief Playback state enumeration
 */
enum class PlaybackState
{
    Stopped,
    Playing,
    Paused,
    Completed,
    Error
};

/**
 * @brief Interface for event playback functionality
 *
 * This interface defines the contract for replaying captured input events.
 * Implementations should be platform-specific and handle low-level event
 * injection.
 */
class IEventPlayer
{
  public:
    using PlaybackCallback = std::function<
      void(PlaybackState, size_t currentIndex, size_t totalEvents)>;
    using EventCallback = std::function<void(const Event&)>;

    virtual ~IEventPlayer() = default;

    /**
     * @brief Load events for playback
     * @param events Vector of events to play
     * @return true if events loaded successfully
     */
    virtual bool loadEvents(std::vector<std::unique_ptr<Event>> events) = 0;

    /**
     * @brief Start playing loaded events
     * @param callback Optional callback for playback progress updates
     * @return true if playback started successfully
     */
    virtual bool startPlayback(PlaybackCallback callback = nullptr) = 0;

    /**
     * @brief Pause current playback
     */
    virtual void pausePlayback() = 0;

    /**
     * @brief Resume paused playback
     */
    virtual void resumePlayback() = 0;

    /**
     * @brief Stop current playback
     */
    virtual void stopPlayback() = 0;

    /**
     * @brief Get current playback state
     * @return current state
     */
    virtual PlaybackState getState() const noexcept = 0;

    /**
     * @brief Set playback speed multiplier
     * @param speed Speed multiplier (1.0 = normal, 2.0 = double speed, 0.5 =
     * half speed)
     */
    virtual void setPlaybackSpeed(double speed) = 0;

    /**
     * @brief Get current playback speed
     * @return speed multiplier
     */
    virtual double getPlaybackSpeed() const noexcept = 0;

    /**
     * @brief Set whether to loop playback
     * @param loop true to enable looping
     */
    virtual void setLoopPlayback(bool loop) = 0;

    /**
     * @brief Check if looping is enabled
     * @return true if looping is enabled
     */
    virtual bool isLoopEnabled() const noexcept = 0;

    /**
     * @brief Get current playback position
     * @return current event index
     */
    virtual size_t getCurrentPosition() const noexcept = 0;

    /**
     * @brief Get total number of events
     * @return total event count
     */
    virtual size_t getTotalEvents() const noexcept = 0;

    /**
     * @brief Seek to specific position in playback
     * @param position event index to seek to
     * @return true if seek was successful
     */
    virtual bool seekToPosition(size_t position) = 0;

    /**
     * @brief Set callback for individual event playback
     * @param callback Function to call before each event is played
     */
    virtual void setEventCallback(EventCallback callback) = 0;

    /**
     * @brief Get the last error message if any operation failed
     * @return error message or empty string if no error
     */
    virtual std::string getLastError() const = 0;
};

} // namespace MouseRecorder::Core
