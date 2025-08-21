// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include "Event.hpp"
#include <functional>
#include <memory>

namespace MouseRecorder::Core
{

/**
 * @brief Interface for event recording functionality
 *
 * This interface defines the contract for capturing input events from the
 * system. Implementations should be platform-specific and handle low-level
 * event capture.
 */
class IEventRecorder
{
  public:
    using EventCallback = std::function<void(std::unique_ptr<Event>)>;

    virtual ~IEventRecorder() = default;

    /**
     * @brief Start recording events
     * @param callback Function to call when an event is captured
     * @return true if recording started successfully, false otherwise
     */
    virtual bool startRecording(EventCallback callback) = 0;

    /**
     * @brief Stop recording events
     */
    virtual void stopRecording() = 0;

    /**
     * @brief Check if currently recording
     * @return true if recording is active
     */
    virtual bool isRecording() const noexcept = 0;

    /**
     * @brief Set whether to capture mouse events
     * @param capture true to capture mouse events
     */
    virtual void setCaptureMouseEvents(bool capture) = 0;

    /**
     * @brief Set whether to capture keyboard events
     * @param capture true to capture keyboard events
     */
    virtual void setCaptureKeyboardEvents(bool capture) = 0;

    /**
     * @brief Set whether to optimize mouse movements
     * When enabled, insignificant mouse movements are filtered out
     * @param optimize true to enable optimization
     */
    virtual void setOptimizeMouseMovements(bool optimize) = 0;

    /**
     * @brief Set minimum distance threshold for mouse movement recording
     * @param threshold minimum distance in pixels
     */
    virtual void setMouseMovementThreshold(int threshold) = 0;

    /**
     * @brief Get the last error message if any operation failed
     * @return error message or empty string if no error
     */
    virtual std::string getLastError() const = 0;
};

} // namespace MouseRecorder::Core
