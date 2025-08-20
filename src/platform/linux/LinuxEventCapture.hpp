// Copyright (c) 2025 JackLee
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include "core/IEventRecorder.hpp"
#include "core/IConfiguration.hpp"
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <set>
#include <vector>
#include <chrono>

namespace MouseRecorder::Platform::Linux
{

/**
 * @brief Linux-specific implementation of event recording using X11
 *
 * This class captures mouse and keyboard events using X11 XInput2 extension
 * for raw input event monitoring.
 */
class LinuxEventCapture : public Core::IEventRecorder
{
  public:
    explicit LinuxEventCapture(const Core::IConfiguration& config);
    ~LinuxEventCapture() override;

    // IEventRecorder interface
    bool startRecording(EventCallback callback) override;
    void stopRecording() override;
    bool isRecording() const noexcept override;
    void setCaptureMouseEvents(bool capture) override;
    void setCaptureKeyboardEvents(bool capture) override;
    void setOptimizeMouseMovements(bool optimize) override;
    void setMouseMovementThreshold(int threshold) override;
    std::string getLastError() const override;

  private:
    /**
     * @brief Initialize X11 connection and XInput2 extension
     * @return true if initialization successful
     */
    bool initializeX11();

    /**
     * @brief Cleanup X11 resources
     */
    void cleanupX11();

    /**
     * @brief Setup XInput2 event masks for raw input
     * @return true if setup successful
     */
    bool setupEventMasks();

    /**
     * @brief Main event loop running in separate thread
     */
    void eventLoop();

    /**
     * @brief Process a raw XInput2 event
     * @param event X11 event
     */
    void processRawEvent(XEvent* event);

    /**
     * @brief Process raw mouse event
     * @param data XInput2 raw event data
     */
    void processRawMouseEvent(XIRawEvent* data);

    /**
     * @brief Process raw keyboard event
     * @param data XInput2 raw event data
     */
    void processRawKeyEvent(XIRawEvent* data);

    /**
     * @brief Convert X11 keycode to key name
     * @param keycode X11 keycode
     * @return human-readable key name
     */
    std::string getKeyName(KeyCode keycode);

    /**
     * @brief Get current mouse position
     * @return current cursor position
     */
    Core::Point getCurrentMousePosition();

    /**
     * @brief Check if mouse movement should be recorded
     * @param newPos new mouse position
     * @return true if movement should be recorded
     */
    bool shouldRecordMouseMovement(const Core::Point& newPos);

    /**
     * @brief Check if the key combination is a stop recording shortcut
     * @param keycode X11 keycode
     * @return true if the key combination is the stop recording shortcut
     */
    bool isStopRecordingShortcut(KeyCode keycode);

    /**
     * @brief Update the currently pressed modifier keys
     * @param keycode X11 keycode
     * @param pressed true if key is pressed, false if released
     */
    void updateModifierState(KeyCode keycode, bool pressed);

    /**
     * @brief Convert X11 keycode and modifiers to Qt key sequence string
     * @param keycode main key code
     * @return Qt-style key sequence string (e.g., "Ctrl+R")
     */
    std::string buildKeySequence(KeyCode keycode);

    /**
     * @brief Check if a keycode is a modifier key
     * @param keycode X11 keycode
     * @return true if the key is a modifier (Ctrl, Shift, Alt)
     */
    bool isModifierKey(KeyCode keycode);

    /**
     * @brief Add event to buffer for potential filtering
     * @param event Event to buffer
     */
    void bufferEvent(std::unique_ptr<Core::Event> event);

    /**
     * @brief Flush buffered events to callback
     */
    void flushEventBuffer();

    /**
     * @brief Remove recent modifier key events from buffer when stop shortcut
     * detected
     */
    void filterRecentModifierEvents();

  private:
    // Configuration reference
    const Core::IConfiguration& m_config;

    // X11 resources
    Display* m_display{nullptr};
    Window m_rootWindow{0};
    int m_xiOpcode{0};

    // Threading
    std::unique_ptr<std::thread> m_eventThread;
    std::atomic<bool> m_recording{false};
    std::atomic<bool> m_shouldStop{false};

    // Configuration
    std::atomic<bool> m_captureMouseEvents{true};
    std::atomic<bool> m_captureKeyboardEvents{true};
    std::atomic<bool> m_optimizeMouseMovements{true};
    std::atomic<int> m_mouseMovementThreshold{5};

    // State tracking
    Core::Point m_lastMousePosition{-1, -1};
    std::atomic<bool> m_hasLastMousePosition{false};

    // Shortcut filtering state
    std::set<KeyCode> m_pressedKeys;
    mutable std::mutex m_keyStateMutex;

    // Buffer for recent events to allow retroactive filtering
    struct BufferedEvent
    {
        std::unique_ptr<Core::Event> event;
        std::chrono::steady_clock::time_point timestamp;
    };

    std::vector<BufferedEvent> m_eventBuffer;
    mutable std::mutex m_bufferMutex;
    static constexpr size_t MAX_BUFFER_SIZE = 10;
    static constexpr std::chrono::milliseconds BUFFER_TIMEOUT{500};

    // Callback and error handling
    EventCallback m_eventCallback;
    mutable std::mutex m_errorMutex;
    std::string m_lastError;

    /**
     * @brief Set last error message in thread-safe manner
     * @param error error message
     */
    void setLastError(const std::string& error);
};

} // namespace MouseRecorder::Platform::Linux
