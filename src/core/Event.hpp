#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <variant>

namespace MouseRecorder::Core
{

/**
 * @brief Enumeration of different event types
 */
enum class EventType
{
    MouseMove,
    MouseClick,
    MouseDoubleClick,
    MouseWheel,
    KeyPress,
    KeyRelease,
    KeyCombination
};

/**
 * @brief Mouse button enumeration
 */
enum class MouseButton
{
    Left,
    Right,
    Middle,
    X1,
    X2
};

/**
 * @brief Keyboard modifier flags
 */
enum class KeyModifier : uint32_t
{
    None = 0,
    Ctrl = 1 << 0,
    Shift = 1 << 1,
    Alt = 1 << 2,
    Meta = 1 << 3
};

inline KeyModifier operator|(KeyModifier a, KeyModifier b)
{
    return static_cast<KeyModifier>(static_cast<uint32_t>(a) |
                                    static_cast<uint32_t>(b));
}

inline KeyModifier operator&(KeyModifier a, KeyModifier b)
{
    return static_cast<KeyModifier>(static_cast<uint32_t>(a) &
                                    static_cast<uint32_t>(b));
}

/**
 * @brief Represents a 2D coordinate
 */
struct Point
{
    int x{0};
    int y{0};

    Point() = default;

    Point(int x, int y) : x(x), y(y) {}

    bool operator==(const Point& other) const noexcept
    {
        return x == other.x && y == other.y;
    }
};

/**
 * @brief Mouse event data
 */
struct MouseEventData
{
    Point position;
    MouseButton button{MouseButton::Left};
    int wheelDelta{0}; // For wheel events
    KeyModifier modifiers{KeyModifier::None};
};

/**
 * @brief Keyboard event data
 */
struct KeyboardEventData
{
    uint32_t keyCode{0};
    std::string keyName;
    KeyModifier modifiers{KeyModifier::None};
    bool isRepeated{false};
};

/**
 * @brief Base event class representing a single input event
 */
class Event
{
  public:
    using TimePoint = std::chrono::steady_clock::time_point;
    using EventData = std::variant<MouseEventData, KeyboardEventData>;

    Event(EventType type,
          EventData data,
          TimePoint timestamp = std::chrono::steady_clock::now());
    virtual ~Event() = default;

    // Getters
    EventType getType() const noexcept
    {
        return m_type;
    }

    const EventData& getData() const noexcept
    {
        return m_data;
    }

    TimePoint getTimestamp() const noexcept
    {
        return m_timestamp;
    }

    // Get duration since epoch in milliseconds for serialization
    uint64_t getTimestampMs() const noexcept;

    // Create event from timestamp in milliseconds
    static TimePoint timestampFromMs(uint64_t timestampMs);

    // Type-safe data accessors
    const MouseEventData* getMouseData() const noexcept;
    const KeyboardEventData* getKeyboardData() const noexcept;

    // Utility methods
    bool isMouseEvent() const noexcept;
    bool isKeyboardEvent() const noexcept;

    // String representation for debugging
    std::string toString() const;

  private:
    EventType m_type;
    EventData m_data;
    TimePoint m_timestamp;
};

/**
 * @brief Factory class for creating events
 */
class EventFactory
{
  public:
    static std::unique_ptr<Event> createMouseMoveEvent(
        const Point& position, KeyModifier modifiers = KeyModifier::None);
    static std::unique_ptr<Event> createMouseClickEvent(
        const Point& position,
        MouseButton button,
        KeyModifier modifiers = KeyModifier::None);
    static std::unique_ptr<Event> createMouseDoubleClickEvent(
        const Point& position,
        MouseButton button,
        KeyModifier modifiers = KeyModifier::None);
    static std::unique_ptr<Event> createMouseWheelEvent(
        const Point& position,
        int wheelDelta,
        KeyModifier modifiers = KeyModifier::None);
    static std::unique_ptr<Event> createKeyPressEvent(
        uint32_t keyCode,
        const std::string& keyName,
        KeyModifier modifiers = KeyModifier::None);
    static std::unique_ptr<Event> createKeyReleaseEvent(
        uint32_t keyCode,
        const std::string& keyName,
        KeyModifier modifiers = KeyModifier::None);
    static std::unique_ptr<Event> createKeyCombinationEvent(
        const std::vector<uint32_t>& keyCodes,
        const std::vector<std::string>& keyNames);
};

} // namespace MouseRecorder::Core
