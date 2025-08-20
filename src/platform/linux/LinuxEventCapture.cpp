// Copyright (c) 2025 JackLee
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "LinuxEventCapture.hpp"
#include "core/Event.hpp"
#include "application/MouseRecorderApp.hpp"
#include <X11/extensions/XTest.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include "core/SpdlogConfig.hpp"
#include <cmath>
#include <algorithm>

namespace MouseRecorder::Platform::Linux
{

LinuxEventCapture::LinuxEventCapture(const Core::IConfiguration& config)
    : m_config(config)
{
    spdlog::debug("LinuxEventCapture: Constructor");
}

LinuxEventCapture::~LinuxEventCapture()
{
    // Avoid logging during destruction if spdlog might be shut down

    spdlog::debug("LinuxEventCapture: Destructor cleanup");

    // Stop recording without logging
    if (m_recording.load())
    {
        m_shouldStop.store(true);
        m_recording.store(false);

        if (m_eventThread && m_eventThread->joinable())
        {
            m_eventThread->join();
            m_eventThread.reset();
        }

        m_eventCallback = nullptr;
    }

    cleanupX11();
}

bool LinuxEventCapture::startRecording(EventCallback callback)
{
    spdlog::info("LinuxEventCapture: Starting recording");

    if (m_recording.load())
    {
        setLastError("Recording is already active");
        return false;
    }

    if (!callback)
    {
        setLastError("Event callback is required");
        return false;
    }

    if (!initializeX11())
    {
        return false;
    }

    if (!setupEventMasks())
    {
        cleanupX11();
        return false;
    }

    m_eventCallback = std::move(callback);
    m_shouldStop.store(false);
    m_recording.store(true);

    // Reset state
    m_hasLastMousePosition.store(false);

    // Start event thread
    m_eventThread =
        std::make_unique<std::thread>(&LinuxEventCapture::eventLoop, this);

    spdlog::info("LinuxEventCapture: Recording started successfully");
    return true;
}

void LinuxEventCapture::stopRecording()
{
    spdlog::info("LinuxEventCapture: Stopping recording");

    if (!m_recording.load())
    {
        return;
    }

    m_shouldStop.store(true);
    m_recording.store(false);

    if (m_eventThread && m_eventThread->joinable())
    {
        m_eventThread->join();
        m_eventThread.reset();
    }

    // Flush any remaining buffered events when stopping
    flushEventBuffer();

    m_eventCallback = nullptr;

    spdlog::info("LinuxEventCapture: Recording stopped");
}

bool LinuxEventCapture::isRecording() const noexcept
{
    return m_recording.load();
}

void LinuxEventCapture::setCaptureMouseEvents(bool capture)
{
    m_captureMouseEvents.store(capture);
    spdlog::debug("LinuxEventCapture: Mouse event capture set to {}", capture);
}

void LinuxEventCapture::setCaptureKeyboardEvents(bool capture)
{
    m_captureKeyboardEvents.store(capture);
    spdlog::debug("LinuxEventCapture: Keyboard event capture set to {}",
                  capture);
}

void LinuxEventCapture::setOptimizeMouseMovements(bool optimize)
{
    m_optimizeMouseMovements.store(optimize);
    spdlog::debug("LinuxEventCapture: Mouse movement optimization set to {}",
                  optimize);
}

void LinuxEventCapture::setMouseMovementThreshold(int threshold)
{
    m_mouseMovementThreshold.store(std::max(0, threshold));
    spdlog::debug("LinuxEventCapture: Mouse movement threshold set to {}",
                  threshold);
}

std::string LinuxEventCapture::getLastError() const
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    return m_lastError;
}

bool LinuxEventCapture::initializeX11()
{
    spdlog::debug("LinuxEventCapture: Initializing X11");

    m_display = XOpenDisplay(nullptr);
    if (!m_display)
    {
        setLastError("Failed to open X11 display");
        return false;
    }

    m_rootWindow = DefaultRootWindow(m_display);

    // Check for XInput2 extension
    int event, error;
    if (!XQueryExtension(
            m_display, "XInputExtension", &m_xiOpcode, &event, &error))
    {
        setLastError("XInput extension not available");
        return false;
    }

    // Check XInput2 version
    int major = 2, minor = 0;
    if (XIQueryVersion(m_display, &major, &minor) == BadRequest)
    {
        setLastError(
            "XInput2 not available. Server supports only version < 2.0");
        return false;
    }

    spdlog::debug("LinuxEventCapture: X11 initialized successfully, XInput2 "
                  "version {}.{}",
                  major,
                  minor);
    return true;
}

void LinuxEventCapture::cleanupX11()
{
    spdlog::debug("LinuxEventCapture: Cleaning up X11 resources");

    if (m_display)
    {
        XCloseDisplay(m_display);
        m_display = nullptr;
    }
}

bool LinuxEventCapture::setupEventMasks()
{
    spdlog::debug("LinuxEventCapture: Setting up event masks");

    XIEventMask evmask;
    unsigned char mask[XIMaskLen(XI_LASTEVENT)] = {0};

    // Set up raw event masks
    XISetMask(mask, XI_RawMotion);
    XISetMask(mask, XI_RawButtonPress);
    XISetMask(mask, XI_RawButtonRelease);
    XISetMask(mask, XI_RawKeyPress);
    XISetMask(mask, XI_RawKeyRelease);

    evmask.deviceid = XIAllMasterDevices;
    evmask.mask_len = sizeof(mask);
    evmask.mask = mask;

    if (XISelectEvents(m_display, m_rootWindow, &evmask, 1) != Success)
    {
        setLastError("Failed to select XInput2 events");
        return false;
    }

    XFlush(m_display);
    spdlog::debug("LinuxEventCapture: Event masks set up successfully");
    return true;
}

void LinuxEventCapture::eventLoop()
{
    spdlog::debug("LinuxEventCapture: Event loop started");

    while (!m_shouldStop.load())
    {
        if (XPending(m_display) > 0)
        {
            XEvent event;
            XNextEvent(m_display, &event);

            if (XGetEventData(m_display, &event.xcookie))
            {
                if (event.xcookie.type == GenericEvent &&
                    event.xcookie.extension == m_xiOpcode)
                {
                    processRawEvent(&event);
                }
                XFreeEventData(m_display, &event.xcookie);
            }
        }
        else
        {
            // Small delay to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }

    spdlog::debug("LinuxEventCapture: Event loop ended");
}

void LinuxEventCapture::processRawEvent(XEvent* event)
{
    XIRawEvent* data = static_cast<XIRawEvent*>(event->xcookie.data);

    switch (data->evtype)
    {
    case XI_RawMotion:
    case XI_RawButtonPress:
    case XI_RawButtonRelease:
        if (m_captureMouseEvents.load())
        {
            processRawMouseEvent(data);
        }
        break;

    case XI_RawKeyPress:
    case XI_RawKeyRelease:
        if (m_captureKeyboardEvents.load())
        {
            processRawKeyEvent(data);
        }
        break;
    }
}

void LinuxEventCapture::processRawMouseEvent(XIRawEvent* data)
{
    Core::Point currentPos = getCurrentMousePosition();

    switch (data->evtype)
    {
    case XI_RawMotion: {
        if (shouldRecordMouseMovement(currentPos))
        {
            auto event = Core::EventFactory::createMouseMoveEvent(currentPos);
            m_eventCallback(std::move(event));
            m_lastMousePosition = currentPos;
            m_hasLastMousePosition.store(true);
        }
        break;
    }

    case XI_RawButtonPress: {
        Core::MouseButton button = Core::MouseButton::Left;
        switch (data->detail)
        {
        case 1:
            button = Core::MouseButton::Left;
            break;
        case 2:
            button = Core::MouseButton::Middle;
            break;
        case 3:
            button = Core::MouseButton::Right;
            break;
        case 8:
            button = Core::MouseButton::X1;
            break;
        case 9:
            button = Core::MouseButton::X2;
            break;
        case 4:
        case 5:
        case 6:
        case 7: {
            // Scroll wheel events
            int wheelDelta =
                (data->detail == 4 || data->detail == 6) ? 120 : -120;
            auto event = Core::EventFactory::createMouseWheelEvent(currentPos,
                                                                   wheelDelta);
            m_eventCallback(std::move(event));
            return;
        }
        default:
            return;
        }

        auto event =
            Core::EventFactory::createMouseClickEvent(currentPos, button);
        m_eventCallback(std::move(event));
        break;
    }

    case XI_RawButtonRelease:
        // For now, we only capture button press events
        // Button release could be added if needed
        break;
    }
}

void LinuxEventCapture::processRawKeyEvent(XIRawEvent* data)
{
    std::string keyName = getKeyName(data->detail);

    switch (data->evtype)
    {
    case XI_RawKeyPress: {
        // Update modifier state
        updateModifierState(data->detail, true);

        // Check if this completes a stop recording shortcut
        if (isStopRecordingShortcut(data->detail))
        {
            spdlog::debug("LinuxEventCapture: Detected stop recording "
                          "shortcut, filtering recent modifiers");
            // Filter out recent modifier events that are part of this shortcut
            filterRecentModifierEvents();
            // Don't record this final key either
            return;
        }

        auto event =
            Core::EventFactory::createKeyPressEvent(data->detail, keyName);

        // If this is a modifier key and filtering is enabled, buffer it briefly
        // in case it's part of a stop shortcut
        if (isModifierKey(data->detail) &&
            m_config.getBool(Core::ConfigKeys::FILTER_STOP_RECORDING_SHORTCUT,
                             true))
        {
            bufferEvent(std::move(event));
        }
        else
        {
            // Flush any buffered events first, then send this one
            flushEventBuffer();
            m_eventCallback(std::move(event));
        }
        break;
    }

    case XI_RawKeyRelease: {
        // Update modifier state
        updateModifierState(data->detail, false);

        // Key releases are processed normally - no special filtering needed
        flushEventBuffer(); // Flush any pending events first
        auto event =
            Core::EventFactory::createKeyReleaseEvent(data->detail, keyName);
        m_eventCallback(std::move(event));
        break;
    }
    }
}

std::string LinuxEventCapture::getKeyName(KeyCode keycode)
{
    if (!m_display)
    {
        return "Unknown";
    }

    KeySym keysym = XkbKeycodeToKeysym(m_display, keycode, 0, 0);
    if (keysym == NoSymbol)
    {
        return "Unknown";
    }

    char* keyName = XKeysymToString(keysym);
    return keyName ? std::string(keyName) : "Unknown";
}

Core::Point LinuxEventCapture::getCurrentMousePosition()
{
    if (!m_display)
    {
        return {0, 0};
    }

    Window root, child;
    int rootX, rootY, winX, winY;
    unsigned int mask;

    if (XQueryPointer(m_display,
                      m_rootWindow,
                      &root,
                      &child,
                      &rootX,
                      &rootY,
                      &winX,
                      &winY,
                      &mask))
    {
        return {rootX, rootY};
    }

    return {0, 0};
}

bool LinuxEventCapture::shouldRecordMouseMovement(const Core::Point& newPos)
{
    if (!m_optimizeMouseMovements.load())
    {
        return true;
    }

    if (!m_hasLastMousePosition.load())
    {
        return true;
    }

    int threshold = m_mouseMovementThreshold.load();
    if (threshold <= 0)
    {
        return true;
    }

    int deltaX = newPos.x - m_lastMousePosition.x;
    int deltaY = newPos.y - m_lastMousePosition.y;
    double distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);

    return distance >= threshold;
}

void LinuxEventCapture::setLastError(const std::string& error)
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    m_lastError = error;
    spdlog::error("LinuxEventCapture: {}", error);
}

bool LinuxEventCapture::isStopRecordingShortcut(KeyCode keycode)
{
    // If shortcut filtering is disabled, never filter
    if (!m_config.getBool(Core::ConfigKeys::FILTER_STOP_RECORDING_SHORTCUT,
                          true))
    {
        return false;
    }

    // Build the current key sequence string including this key
    auto currentSequence = buildKeySequence(keycode);

    if (currentSequence.empty())
    {
        return false;
    }

    // Only check against the stop recording shortcut
    auto stopRecording = m_config.getString(
        Core::ConfigKeys::SHORTCUT_STOP_RECORDING, "Ctrl+Shift+R");

    bool isStopShortcut = (currentSequence == stopRecording);

    if (isStopShortcut)
    {
        spdlog::debug("LinuxEventCapture: Detected stop recording shortcut: {}",
                      currentSequence);
    }

    return isStopShortcut;
}

bool LinuxEventCapture::isModifierKey(KeyCode keycode)
{
    if (!m_display)
    {
        return false;
    }

    auto keysym = XkbKeycodeToKeysym(m_display, keycode, 0, 0);

    switch (keysym)
    {
    case XK_Control_L:
    case XK_Control_R:
    case XK_Shift_L:
    case XK_Shift_R:
    case XK_Alt_L:
    case XK_Alt_R:
        return true;
    default:
        return false;
    }
}

void LinuxEventCapture::updateModifierState(KeyCode keycode, bool pressed)
{
    std::lock_guard<std::mutex> lock(m_keyStateMutex);

    if (pressed)
    {
        m_pressedKeys.insert(keycode);
        return;
    }
    m_pressedKeys.erase(keycode);
}

std::string LinuxEventCapture::buildKeySequence(KeyCode keycode)
{
    if (!m_display)
    {
        return "";
    }

    std::lock_guard<std::mutex> lock(m_keyStateMutex);

    // Get the key symbol for the main key
    KeySym keysym = XkbKeycodeToKeysym(m_display, keycode, 0, 0);
    if (keysym == NoSymbol)
    {
        return "";
    }

    char* keyName = XKeysymToString(keysym);
    if (!keyName)
    {
        return "";
    }

    // Check for modifier keys currently pressed (including the current key if
    // it's a modifier)
    bool hasCtrl = false;
    bool hasShift = false;
    bool hasAlt = false;
    bool isModifierKey = false;

    // Check if the current key is a modifier
    switch (keysym)
    {
    case XK_Control_L:
    case XK_Control_R:
        hasCtrl = true;
        isModifierKey = true;
        break;
    case XK_Shift_L:
    case XK_Shift_R:
        hasShift = true;
        isModifierKey = true;
        break;
    case XK_Alt_L:
    case XK_Alt_R:
        hasAlt = true;
        isModifierKey = true;
        break;
    }

    // If this is a modifier key press, don't create a sequence
    if (isModifierKey)
    {
        return "";
    }

    // Check for modifiers in the currently pressed keys
    for (KeyCode pressedKey : m_pressedKeys)
    {
        if (pressedKey == keycode)
            continue; // Skip the current key

        KeySym pressedSym = XkbKeycodeToKeysym(m_display, pressedKey, 0, 0);

        switch (pressedSym)
        {
        case XK_Control_L:
        case XK_Control_R:
            hasCtrl = true;
            break;
        case XK_Shift_L:
        case XK_Shift_R:
            hasShift = true;
            break;
        case XK_Alt_L:
        case XK_Alt_R:
            hasAlt = true;
            break;
        }
    }

    // Only create sequences for non-modifier keys with at least one modifier
    if (!hasCtrl && !hasShift && !hasAlt)
    {
        return "";
    }

    std::string result;

    // Build the sequence string in Qt format
    if (hasCtrl)
        result += "Ctrl+";
    if (hasShift)
        result += "Shift+";
    if (hasAlt)
        result += "Alt+";

    // Convert key name to proper case for Qt format
    std::string key = std::string(keyName);
    if (key.length() == 1)
    {
        key[0] = std::toupper(key[0]);
    }

    result += key;

    return result;
}

void LinuxEventCapture::bufferEvent(std::unique_ptr<Core::Event> event)
{
    std::lock_guard<std::mutex> lock(m_bufferMutex);

    // Add event to buffer with current timestamp
    BufferedEvent bufferedEvent;
    bufferedEvent.event = std::move(event);
    bufferedEvent.timestamp = std::chrono::steady_clock::now();

    m_eventBuffer.push_back(std::move(bufferedEvent));

    // Remove old events beyond buffer size
    if (m_eventBuffer.size() > MAX_BUFFER_SIZE)
    {
        m_eventBuffer.erase(m_eventBuffer.begin());
    }

    // Remove events older than timeout
    auto now = std::chrono::steady_clock::now();
    m_eventBuffer.erase(std::remove_if(m_eventBuffer.begin(),
                                       m_eventBuffer.end(),
                                       [now](const BufferedEvent& buffered)
                                       {
                                           return (now - buffered.timestamp) >
                                                  BUFFER_TIMEOUT;
                                       }),
                        m_eventBuffer.end());
}

void LinuxEventCapture::flushEventBuffer()
{
    std::lock_guard<std::mutex> lock(m_bufferMutex);

    // Send all buffered events to callback
    for (auto& bufferedEvent : m_eventBuffer)
    {
        if (bufferedEvent.event && m_eventCallback)
        {
            m_eventCallback(std::move(bufferedEvent.event));
        }
    }

    m_eventBuffer.clear();
}

void LinuxEventCapture::filterRecentModifierEvents()
{
    std::lock_guard<std::mutex> lock(m_bufferMutex);

    // Remove recent modifier key events from buffer
    auto now = std::chrono::steady_clock::now();

    m_eventBuffer.erase(
        std::remove_if(m_eventBuffer.begin(),
                       m_eventBuffer.end(),
                       [this, now](const BufferedEvent& buffered)
                       {
                           // Remove if it's a recent modifier key event
                           if ((now - buffered.timestamp) > BUFFER_TIMEOUT)
                           {
                               return false; // Keep old events
                           }

                           if (!buffered.event ||
                               !buffered.event->isKeyboardEvent())
                           {
                               return false; // Keep non-keyboard events
                           }

                           const Core::KeyboardEventData* keyData =
                               buffered.event->getKeyboardData();
                           if (!keyData)
                           {
                               return false;
                           }

                           // Check if this is a modifier key that should be
                           // filtered
                           return isModifierKey(keyData->keyCode);
                       }),
        m_eventBuffer.end());

    spdlog::debug(
        "LinuxEventCapture: Filtered recent modifier events from buffer");
}

} // namespace MouseRecorder::Platform::Linux
