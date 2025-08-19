#include "WindowsEventCapture.hpp"

namespace MouseRecorder::Platform::Windows
{

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
}

WindowsEventCapture::~WindowsEventCapture()
{
    stopRecording();
}

bool WindowsEventCapture::startRecording(EventCallback callback)
{
    // Suppress unused parameter warning for stub implementation
    (void) callback;

    // Stub: Only implemented on Windows
    setLastError("Not implemented on non-Windows platforms");
    return false;
}

void WindowsEventCapture::stopRecording()
{
    // Stub
}

bool WindowsEventCapture::isRecording() const noexcept
{
    return false;
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
}

} // namespace MouseRecorder::Platform::Windows
