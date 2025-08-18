#include "Event.hpp"
#include <sstream>
#include <iomanip>

namespace MouseRecorder::Core
{

Event::Event(EventType type, EventData data, TimePoint timestamp)
    : m_type(type), m_data(std::move(data)), m_timestamp(timestamp)
{}

uint64_t Event::getTimestampMs() const noexcept
{
    auto duration = m_timestamp.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration)
        .count();
}

Event::TimePoint Event::timestampFromMs(uint64_t timestampMs)
{
    auto duration = std::chrono::milliseconds(timestampMs);
    return TimePoint(duration);
}

const MouseEventData* Event::getMouseData() const noexcept
{
    return std::get_if<MouseEventData>(&m_data);
}

const KeyboardEventData* Event::getKeyboardData() const noexcept
{
    return std::get_if<KeyboardEventData>(&m_data);
}

bool Event::isMouseEvent() const noexcept
{
    return getMouseData() != nullptr;
}

bool Event::isKeyboardEvent() const noexcept
{
    return getKeyboardData() != nullptr;
}

std::string Event::toString() const
{
    std::ostringstream oss;
    oss << "Event[";

    switch (m_type)
    {
    case EventType::MouseMove:
        oss << "MouseMove";
        break;
    case EventType::MouseClick:
        oss << "MouseClick";
        break;
    case EventType::MouseDoubleClick:
        oss << "MouseDoubleClick";
        break;
    case EventType::MouseWheel:
        oss << "MouseWheel";
        break;
    case EventType::KeyPress:
        oss << "KeyPress";
        break;
    case EventType::KeyRelease:
        oss << "KeyRelease";
        break;
    case EventType::KeyCombination:
        oss << "KeyCombination";
        break;
    }

    oss << ", timestamp=" << getTimestampMs();

    if (auto mouseData = getMouseData())
    {
        oss << ", pos=(" << mouseData->position.x << ","
            << mouseData->position.y << ")";
        if (m_type == EventType::MouseClick ||
            m_type == EventType::MouseDoubleClick)
        {
            oss << ", button=";
            switch (mouseData->button)
            {
            case MouseButton::Left:
                oss << "Left";
                break;
            case MouseButton::Right:
                oss << "Right";
                break;
            case MouseButton::Middle:
                oss << "Middle";
                break;
            case MouseButton::X1:
                oss << "X1";
                break;
            case MouseButton::X2:
                oss << "X2";
                break;
            }
        }
        if (m_type == EventType::MouseWheel)
        {
            oss << ", wheelDelta=" << mouseData->wheelDelta;
        }
    }

    if (auto keyData = getKeyboardData())
    {
        oss << ", key=" << keyData->keyName << " (code=" << keyData->keyCode
            << ")";
        if (keyData->isRepeated)
        {
            oss << ", repeated";
        }
    }

    oss << "]";
    return oss.str();
}

// EventFactory implementations
std::unique_ptr<Event> EventFactory::createMouseMoveEvent(const Point& position,
                                                          KeyModifier modifiers)
{
    MouseEventData data;
    data.position = position;
    data.modifiers = modifiers;
    return std::make_unique<Event>(EventType::MouseMove, data);
}

std::unique_ptr<Event> EventFactory::createMouseClickEvent(
    const Point& position, MouseButton button, KeyModifier modifiers)
{
    MouseEventData data;
    data.position = position;
    data.button = button;
    data.modifiers = modifiers;
    return std::make_unique<Event>(EventType::MouseClick, data);
}

std::unique_ptr<Event> EventFactory::createMouseDoubleClickEvent(
    const Point& position, MouseButton button, KeyModifier modifiers)
{
    MouseEventData data;
    data.position = position;
    data.button = button;
    data.modifiers = modifiers;
    return std::make_unique<Event>(EventType::MouseDoubleClick, data);
}

std::unique_ptr<Event> EventFactory::createMouseWheelEvent(
    const Point& position, int wheelDelta, KeyModifier modifiers)
{
    MouseEventData data;
    data.position = position;
    data.wheelDelta = wheelDelta;
    data.modifiers = modifiers;
    return std::make_unique<Event>(EventType::MouseWheel, data);
}

std::unique_ptr<Event> EventFactory::createKeyPressEvent(
    uint32_t keyCode, const std::string& keyName, KeyModifier modifiers)
{
    KeyboardEventData data;
    data.keyCode = keyCode;
    data.keyName = keyName;
    data.modifiers = modifiers;
    return std::make_unique<Event>(EventType::KeyPress, data);
}

std::unique_ptr<Event> EventFactory::createKeyReleaseEvent(
    uint32_t keyCode, const std::string& keyName, KeyModifier modifiers)
{
    KeyboardEventData data;
    data.keyCode = keyCode;
    data.keyName = keyName;
    data.modifiers = modifiers;
    return std::make_unique<Event>(EventType::KeyRelease, data);
}

std::unique_ptr<Event> EventFactory::createKeyCombinationEvent(
    const std::vector<uint32_t>& keyCodes,
    const std::vector<std::string>& keyNames)
{
    KeyboardEventData data;
    if (!keyCodes.empty())
    {
        data.keyCode = keyCodes[0]; // Primary key
    }
    if (!keyNames.empty())
    {
        // Combine all key names
        std::ostringstream oss;
        for (size_t i = 0; i < keyNames.size(); ++i)
        {
            if (i > 0)
                oss << "+";
            oss << keyNames[i];
        }
        data.keyName = oss.str();
    }
    return std::make_unique<Event>(EventType::KeyCombination, data);
}

} // namespace MouseRecorder::Core
