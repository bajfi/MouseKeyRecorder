#include "WindowsEventCapture.hpp"
#include "core/Event.hpp"
#include "core/SpdlogConfig.hpp"
#include <algorithm>
#include <cmath>

namespace MouseRecorder::Platform::Windows
{

// Static member initialization
WindowsEventCapture* WindowsEventCapture::s_instance = nullptr;
std::mutex WindowsEventCapture::s_instanceMutex;

WindowsEventCapture::WindowsEventCapture(const Core::IConfiguration& config)
    : m_config(config),
      m_mouseHook(nullptr),
      m_keyboardHook(nullptr),
      m_recording(false),
      m_captureMouseEvents(true),
      m_captureKeyboardEvents(true),
      m_optimizeMouseMovements(true),
      m_mouseMovementThreshold(5),
      m_shouldStopMessageLoop(false)
{
    m_lastMousePosition = {0, 0};
    m_lastMouseTime = std::chrono::steady_clock::now();
}

WindowsEventCapture::~WindowsEventCapture()
{
    stopRecording();
}

bool WindowsEventCapture::startRecording(EventCallback callback)
{
    if (m_recording.load())
    {
        setLastError("Recording is already in progress");
        return false;
    }

    if (!callback)
    {
        setLastError("Invalid callback provided");
        return false;
    }

    // Set callback
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_eventCallback = callback;
    }

    // Set static instance for hook procedures
    {
        std::lock_guard<std::mutex> lock(s_instanceMutex);
        s_instance = this;
    }

    // Initialize hooks
    if (!initializeHooks())
    {
        std::lock_guard<std::mutex> lock(s_instanceMutex);
        s_instance = nullptr;
        return false;
    }

    m_recording.store(true);

    // Start message loop in separate thread
    startMessageLoop();

    spdlog::info("WindowsEventCapture: Recording started successfully");
    return true;
}

void WindowsEventCapture::stopRecording()
{
    if (!m_recording.load())
    {
        return;
    }

    spdlog::info("WindowsEventCapture: Stopping recording");

    m_recording.store(false);

    // Stop message loop
    stopMessageLoop();

    // Clean up hooks
    cleanupHooks();

    // Clear static instance
    {
        std::lock_guard<std::mutex> lock(s_instanceMutex);
        s_instance = nullptr;
    }

    // Clear callback
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_eventCallback = nullptr;
    }

    spdlog::info("WindowsEventCapture: Recording stopped successfully");
}

bool WindowsEventCapture::isRecording() const noexcept
{
    return m_recording.load();
}

void WindowsEventCapture::setCaptureMouseEvents(bool capture)
{
    m_captureMouseEvents = capture;
}

void WindowsEventCapture::setCaptureKeyboardEvents(bool capture)
{
    m_captureKeyboardEvents = capture;
}

void WindowsEventCapture::setOptimizeMouseMovements(bool optimize)
{
    m_optimizeMouseMovements = optimize;
}

void WindowsEventCapture::setMouseMovementThreshold(int threshold)
{
    m_mouseMovementThreshold = threshold;
}

std::string WindowsEventCapture::getLastError() const
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    return m_lastError;
}

void WindowsEventCapture::setLastError(const std::string& error)
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    m_lastError = error;
    spdlog::error("WindowsEventCapture: {}", error);
}

bool WindowsEventCapture::initializeHooks()
{
    if (m_captureMouseEvents.load())
    {
        m_mouseHook = SetWindowsHookEx(
            WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(nullptr), 0);
        if (!m_mouseHook)
        {
            DWORD error = GetLastError();
            setLastError("Failed to install mouse hook. Error: " +
                         std::to_string(error));
            return false;
        }
        spdlog::debug("WindowsEventCapture: Mouse hook installed");
    }

    if (m_captureKeyboardEvents.load())
    {
        m_keyboardHook = SetWindowsHookEx(
            WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(nullptr), 0);
        if (!m_keyboardHook)
        {
            DWORD error = GetLastError();
            setLastError("Failed to install keyboard hook. Error: " +
                         std::to_string(error));
            cleanupHooks(); // Clean up mouse hook if it was installed
            return false;
        }
        spdlog::debug("WindowsEventCapture: Keyboard hook installed");
    }

    return true;
}

void WindowsEventCapture::cleanupHooks()
{
    if (m_mouseHook)
    {
        UnhookWindowsHookEx(m_mouseHook);
        m_mouseHook = nullptr;
        spdlog::debug("WindowsEventCapture: Mouse hook removed");
    }

    if (m_keyboardHook)
    {
        UnhookWindowsHookEx(m_keyboardHook);
        m_keyboardHook = nullptr;
        spdlog::debug("WindowsEventCapture: Keyboard hook removed");
    }
}

void WindowsEventCapture::startMessageLoop()
{
    m_shouldStopMessageLoop.store(false);
    m_messageLoopThread = std::make_unique<std::thread>(
        [this]()
        {
            spdlog::debug("WindowsEventCapture: Message loop thread started");

            MSG msg;
            while (!m_shouldStopMessageLoop.load() && m_recording.load())
            {
                BOOL result = GetMessage(&msg, nullptr, 0, 0);
                if (result == -1) // Error
                {
                    DWORD error = GetLastError();
                    spdlog::error(
                        "WindowsEventCapture: GetMessage failed with error: {}",
                        error);
                    break;
                }
                else if (result == 0) // WM_QUIT
                {
                    spdlog::debug("WindowsEventCapture: Received WM_QUIT");
                    break;
                }

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            spdlog::debug("WindowsEventCapture: Message loop thread ended");
        });
}

void WindowsEventCapture::stopMessageLoop()
{
    if (m_messageLoopThread && m_messageLoopThread->joinable())
    {
        m_shouldStopMessageLoop.store(true);

        // Post a message to wake up the message loop
        PostThreadMessage(
            GetThreadId(m_messageLoopThread->native_handle()), WM_QUIT, 0, 0);

        m_messageLoopThread->join();
        m_messageLoopThread.reset();
        spdlog::debug("WindowsEventCapture: Message loop thread stopped");
    }
}

LRESULT CALLBACK WindowsEventCapture::LowLevelMouseProc(int nCode,
                                                        WPARAM wParam,
                                                        LPARAM lParam)
{
    if (nCode >= 0)
    {
        std::lock_guard<std::mutex> lock(s_instanceMutex);
        if (s_instance && s_instance->m_recording.load() &&
            s_instance->m_captureMouseEvents.load())
        {
            auto* mouseStruct = reinterpret_cast<const MSLLHOOKSTRUCT*>(lParam);
            s_instance->processMouseEvent(wParam, mouseStruct);
        }
    }

    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT CALLBACK WindowsEventCapture::LowLevelKeyboardProc(int nCode,
                                                           WPARAM wParam,
                                                           LPARAM lParam)
{
    if (nCode >= 0)
    {
        std::lock_guard<std::mutex> lock(s_instanceMutex);
        if (s_instance && s_instance->m_recording.load() &&
            s_instance->m_captureKeyboardEvents.load())
        {
            auto* keyboardStruct =
                reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
            s_instance->processKeyboardEvent(wParam, keyboardStruct);
        }
    }

    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void WindowsEventCapture::processMouseEvent(WPARAM wParam,
                                            const MSLLHOOKSTRUCT* mouseStruct)
{
    try
    {
        Core::Point position{mouseStruct->pt.x, mouseStruct->pt.y};

        // Get current modifiers (simplified - you might want to check actual
        // key states)
        Core::KeyModifier modifiers = Core::KeyModifier::None;

        std::unique_ptr<Core::Event> event;

        switch (wParam)
        {
        case WM_MOUSEMOVE:
            if (m_optimizeMouseMovements.load() &&
                shouldOptimizeMouseMovement(position.x, position.y))
            {
                return; // Skip this mouse movement
            }
            event =
                Core::EventFactory::createMouseMoveEvent(position, modifiers);
            break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            event = Core::EventFactory::createMouseClickEvent(
                position, Core::MouseButton::Left, modifiers);
            break;

        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            event = Core::EventFactory::createMouseClickEvent(
                position, Core::MouseButton::Right, modifiers);
            break;

        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            event = Core::EventFactory::createMouseClickEvent(
                position, Core::MouseButton::Middle, modifiers);
            break;

        case WM_MOUSEWHEEL: {
            int wheelDelta = GET_WHEEL_DELTA_WPARAM(mouseStruct->mouseData);
            event = Core::EventFactory::createMouseWheelEvent(
                position, wheelDelta, modifiers);
            break;
        }

        default:
            return; // Unknown mouse event
        }

        // Send event to callback
        if (event)
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            if (m_eventCallback)
            {
                m_eventCallback(std::move(event));
            }
        }
    }
    catch (const std::exception& e)
    {
        spdlog::error("WindowsEventCapture: Error processing mouse event: {}",
                      e.what());
    }
}

void WindowsEventCapture::processKeyboardEvent(
    WPARAM wParam, const KBDLLHOOKSTRUCT* keyboardStruct)
{
    try
    {
        uint32_t keyCode = static_cast<uint32_t>(keyboardStruct->vkCode);

        // Convert virtual key to name (simplified mapping)
        std::string keyName;
        if (keyCode >= 'A' && keyCode <= 'Z')
        {
            keyName = static_cast<char>(keyCode);
        }
        else if (keyCode >= '0' && keyCode <= '9')
        {
            keyName = static_cast<char>(keyCode);
        }
        else
        {
            // For other keys, use the virtual key code as string
            keyName = "VK_" + std::to_string(keyCode);
        }

        // Get current modifiers (simplified)
        Core::KeyModifier modifiers = Core::KeyModifier::None;

        std::unique_ptr<Core::Event> event;

        switch (wParam)
        {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            event = Core::EventFactory::createKeyPressEvent(
                keyCode, keyName, modifiers);
            break;

        case WM_KEYUP:
        case WM_SYSKEYUP:
            event = Core::EventFactory::createKeyReleaseEvent(
                keyCode, keyName, modifiers);
            break;

        default:
            return; // Unknown keyboard event
        }

        // Send event to callback
        if (event)
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            if (m_eventCallback)
            {
                m_eventCallback(std::move(event));
            }
        }
    }
    catch (const std::exception& e)
    {
        spdlog::error(
            "WindowsEventCapture: Error processing keyboard event: {}",
            e.what());
    }
}

bool WindowsEventCapture::shouldOptimizeMouseMovement(int x, int y)
{
    if (!m_optimizeMouseMovements.load())
    {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    int threshold = m_mouseMovementThreshold.load();

    // Calculate distance from last position
    int dx = x - m_lastMousePosition.x;
    int dy = y - m_lastMousePosition.y;
    double distance = std::sqrt(dx * dx + dy * dy);

    // Update last position and time
    m_lastMousePosition.x = x;
    m_lastMousePosition.y = y;
    m_lastMouseTime = now;

    // Skip if movement is below threshold
    return distance < threshold;
}

} // namespace MouseRecorder::Platform::Windows
