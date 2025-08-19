#pragma once

#include "core/IEventRecorder.hpp"
#include "core/IConfiguration.hpp"
#include <windows.h>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <chrono>

namespace MouseRecorder::Platform::Windows
{

/**
 * @brief Windows-specific implementation of event recording using Win32 API
 *
 * This class captures mouse and keyboard events using Windows low-level hooks
 * (SetWindowsHookEx with WH_MOUSE_LL and WH_KEYBOARD_LL).
 */
class WindowsEventCapture : public Core::IEventRecorder
{
  public:
    explicit WindowsEventCapture(const Core::IConfiguration& config);
    ~WindowsEventCapture() override;

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
     * @brief Initialize Windows hooks for event capture
     * @return true if initialization successful
     */
    bool initializeHooks();

    /**
     * @brief Cleanup Windows hooks
     */
    void cleanupHooks();

    /**
     * @brief Start the message loop in a separate thread
     */
    void startMessageLoop();

    /**
     * @brief Stop the message loop and clean up
     */
    void stopMessageLoop();

    /**
     * @brief Low-level mouse hook procedure
     */
    static LRESULT CALLBACK LowLevelMouseProc(int nCode,
                                              WPARAM wParam,
                                              LPARAM lParam);

    /**
     * @brief Low-level keyboard hook procedure
     */
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode,
                                                 WPARAM wParam,
                                                 LPARAM lParam);

    /**
     * @brief Process mouse event and create Event object
     */
    void processMouseEvent(WPARAM wParam, const MSLLHOOKSTRUCT* mouseStruct);

    /**
     * @brief Process keyboard event and create Event object
     */
    void processKeyboardEvent(WPARAM wParam,
                              const KBDLLHOOKSTRUCT* keyboardStruct);

    /**
     * @brief Convert Windows virtual key code to Qt key code
     */
    int convertVirtualKeyToQt(DWORD vkCode) const;

    /**
     * @brief Get mouse button from Windows message
     */
    Core::MouseButton getMouseButtonFromMessage(WPARAM wParam) const;

    /**
     * @brief Check if mouse movement should be optimized away
     */
    bool shouldOptimizeMouseMovement(int x, int y);

    /**
     * @brief Set last error message
     */
    void setLastError(const std::string& error);

    // Configuration
    const Core::IConfiguration& m_config;

    // Hook handles
    HHOOK m_mouseHook;
    HHOOK m_keyboardHook;

    // State
    std::atomic<bool> m_recording;
    std::atomic<bool> m_captureMouseEvents;
    std::atomic<bool> m_captureKeyboardEvents;
    std::atomic<bool> m_optimizeMouseMovements;
    std::atomic<int> m_mouseMovementThreshold;

    // Threading
    std::unique_ptr<std::thread> m_messageLoopThread;
    std::atomic<bool> m_shouldStopMessageLoop;

    // Callback
    EventCallback m_eventCallback;
    std::mutex m_callbackMutex;

    // Mouse optimization
    POINT m_lastMousePosition;
    std::chrono::steady_clock::time_point m_lastMouseTime;

    // Error handling
    mutable std::mutex m_errorMutex;
    std::string m_lastError;

    // Static instance for hook procedures
    static WindowsEventCapture* s_instance;
    static std::mutex s_instanceMutex;
};

} // namespace MouseRecorder::Platform::Windows
