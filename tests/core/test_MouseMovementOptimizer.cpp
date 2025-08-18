#include <gtest/gtest.h>
#include "core/MouseMovementOptimizer.hpp"
#include "core/Event.hpp"
#include <vector>
#include <memory>
#include <chrono>

using namespace MouseRecorder::Core;

class MouseMovementOptimizerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Common test setup
    }

    void TearDown() override
    {
        // Common test cleanup
    }

    // Helper function to create a mouse move event
    std::unique_ptr<Event> createMouseMoveEvent(
        int x,
        int y,
        std::chrono::milliseconds offsetMs = std::chrono::milliseconds(0))
    {
        auto baseTime = std::chrono::steady_clock::now();
        auto timestamp = baseTime + offsetMs;

        MouseEventData mouseData;
        mouseData.position = Point(x, y);

        return std::make_unique<Event>(
            EventType::MouseMove, mouseData, timestamp);
    }

    // Helper function to create a mouse click event
    std::unique_ptr<Event> createMouseClickEvent(
        int x,
        int y,
        std::chrono::milliseconds offsetMs = std::chrono::milliseconds(0))
    {
        auto baseTime = std::chrono::steady_clock::now();
        auto timestamp = baseTime + offsetMs;

        MouseEventData mouseData;
        mouseData.position = Point(x, y);
        mouseData.button = MouseButton::Left;

        return std::make_unique<Event>(
            EventType::MouseClick, mouseData, timestamp);
    }

    // Helper to create a sequence of mouse moves along a line
    std::vector<std::unique_ptr<Event>> createLinearMouseSequence(
        int startX,
        int startY,
        int endX,
        int endY,
        int steps,
        int timeIntervalMs = 16)
    {
        std::vector<std::unique_ptr<Event>> events;

        double deltaX = static_cast<double>(endX - startX) / (steps - 1);
        double deltaY = static_cast<double>(endY - startY) / (steps - 1);

        for (int i = 0; i < steps; ++i)
        {
            int x = static_cast<int>(startX + i * deltaX);
            int y = static_cast<int>(startY + i * deltaY);

            events.push_back(createMouseMoveEvent(
                x, y, std::chrono::milliseconds(i * timeIntervalMs)));
        }

        return events;
    }
};

TEST_F(MouseMovementOptimizerTest, ExtractMouseMoveEvents)
{
    std::vector<std::unique_ptr<Event>> events;

    // Add mixed events
    events.push_back(createMouseMoveEvent(0, 0));
    events.push_back(createMouseClickEvent(0, 0));
    events.push_back(createMouseMoveEvent(10, 10));
    events.push_back(createMouseMoveEvent(20, 20));

    auto mouseMoves = MouseMovementOptimizer::extractMouseMoveEvents(events);

    EXPECT_EQ(mouseMoves.size(), 3);
    EXPECT_EQ(mouseMoves[0].first, 0); // First event index
    EXPECT_EQ(mouseMoves[1].first, 2); // Third event index
    EXPECT_EQ(mouseMoves[2].first, 3); // Fourth event index
}

TEST_F(MouseMovementOptimizerTest, CalculateDistance)
{
    Point p1(0, 0);
    Point p2(3, 4);

    double distance = MouseMovementOptimizer::calculateDistance(p1, p2);
    EXPECT_DOUBLE_EQ(distance, 5.0); // 3-4-5 triangle
}

TEST_F(MouseMovementOptimizerTest, PerpendicularDistance)
{
    Point lineStart(0, 0);
    Point lineEnd(10, 0);
    Point point(5, 3);

    double distance = MouseMovementOptimizer::perpendicularDistance(
        point, lineStart, lineEnd);
    EXPECT_DOUBLE_EQ(distance, 3.0);
}

TEST_F(MouseMovementOptimizerTest, DistanceThresholdOptimization)
{
    auto events =
        createLinearMouseSequence(0, 0, 100, 0, 21); // 21 points, 5px apart

    MouseMovementOptimizer::OptimizationConfig config;
    config.strategy =
        MouseMovementOptimizer::OptimizationStrategy::DistanceThreshold;
    config.distanceThreshold = 10; // Remove points closer than 10px

    size_t originalSize = events.size();
    size_t removedCount =
        MouseMovementOptimizer::optimizeEvents(events, config);

    EXPECT_GT(removedCount, 0);
    EXPECT_LT(events.size(), originalSize);

    // First and last points should be preserved
    const auto* firstEvent = events.front()->getMouseData();
    const auto* lastEvent = events.back()->getMouseData();
    EXPECT_EQ(firstEvent->position.x, 0);
    EXPECT_EQ(lastEvent->position.x, 100);
}

TEST_F(MouseMovementOptimizerTest, TimeBasedOptimization)
{
    std::vector<std::unique_ptr<Event>> events;

    // Create events with 8ms intervals (faster than 60fps)
    for (int i = 0; i < 10; ++i)
    {
        events.push_back(
            createMouseMoveEvent(i * 10, 0, std::chrono::milliseconds(i * 8)));
    }

    MouseMovementOptimizer::OptimizationConfig config;
    config.strategy = MouseMovementOptimizer::OptimizationStrategy::TimeBased;
    config.timeThresholdMs = 16; // Remove events faster than 60fps

    size_t originalSize = events.size();
    size_t removedCount =
        MouseMovementOptimizer::optimizeEvents(events, config);

    EXPECT_GT(removedCount, 0);
    EXPECT_LT(events.size(), originalSize);
}

TEST_F(MouseMovementOptimizerTest, DouglasPeuckerOptimization)
{
    // Create a straight line with some noise
    std::vector<std::unique_ptr<Event>> events;
    events.push_back(createMouseMoveEvent(0, 0));
    events.push_back(createMouseMoveEvent(10, 1)); // slight deviation
    events.push_back(createMouseMoveEvent(20, 0));
    events.push_back(createMouseMoveEvent(30, 1)); // slight deviation
    events.push_back(createMouseMoveEvent(40, 0));
    events.push_back(createMouseMoveEvent(50, 0));

    MouseMovementOptimizer::OptimizationConfig config;
    config.strategy =
        MouseMovementOptimizer::OptimizationStrategy::DouglasPeucker;
    config.douglasPeuckerEpsilon = 0.5; // Remove deviations less than 0.5px

    size_t originalSize = events.size();
    size_t removedCount =
        MouseMovementOptimizer::optimizeEvents(events, config);

    EXPECT_GT(removedCount, 0);
    EXPECT_LT(events.size(), originalSize);

    // First and last should be preserved
    const auto* firstEvent = events.front()->getMouseData();
    const auto* lastEvent = events.back()->getMouseData();
    EXPECT_EQ(firstEvent->position.x, 0);
    EXPECT_EQ(lastEvent->position.x, 50);
}

TEST_F(MouseMovementOptimizerTest, PreserveClickAdjacent)
{
    std::vector<std::unique_ptr<Event>> events;

    // Create sequence: move -> move -> click -> move -> move
    events.push_back(createMouseMoveEvent(0, 0));
    events.push_back(
        createMouseMoveEvent(1, 0)); // Should be preserved (before click)
    events.push_back(createMouseClickEvent(2, 0));
    events.push_back(
        createMouseMoveEvent(3, 0)); // Should be preserved (after click)
    events.push_back(createMouseMoveEvent(4, 0));

    MouseMovementOptimizer::OptimizationConfig config;
    config.strategy =
        MouseMovementOptimizer::OptimizationStrategy::DistanceThreshold;
    config.distanceThreshold = 2;
    config.preserveClicks = true;

    MouseMovementOptimizer::optimizeEvents(events, config);

    // Should preserve click-adjacent moves
    EXPECT_GE(events.size(), 3); // At least click + adjacent moves

    // Verify the click is still there
    bool hasClick = false;
    for (const auto& event : events)
    {
        if (event->getType() == EventType::MouseClick)
        {
            hasClick = true;
            break;
        }
    }
    EXPECT_TRUE(hasClick);
}

TEST_F(MouseMovementOptimizerTest, CombinedOptimization)
{
    // Create a complex sequence with redundant movements
    std::vector<std::unique_ptr<Event>> events;

    // Rapid movements with small distances and short time intervals
    for (int i = 0; i < 20; ++i)
    {
        events.push_back(
            createMouseMoveEvent(i,
                                 i % 3,
                                 std::chrono::milliseconds(
                                     i * 8))); // 8ms intervals, small movements
    }

    MouseMovementOptimizer::OptimizationConfig config;
    config.strategy = MouseMovementOptimizer::OptimizationStrategy::Combined;
    config.distanceThreshold = 3;
    config.timeThresholdMs = 16;
    config.douglasPeuckerEpsilon = 1.0;

    size_t originalSize = events.size();
    size_t removedCount =
        MouseMovementOptimizer::optimizeEvents(events, config);

    EXPECT_GT(removedCount, 0);
    EXPECT_LT(events.size(), originalSize);

    // Should significantly reduce the number of events
    EXPECT_LT(events.size(), originalSize / 2);
}

TEST_F(MouseMovementOptimizerTest, DisabledOptimization)
{
    auto events = createLinearMouseSequence(0, 0, 100, 0, 21);

    MouseMovementOptimizer::OptimizationConfig config;
    config.enabled = false;

    size_t originalSize = events.size();
    size_t removedCount =
        MouseMovementOptimizer::optimizeEvents(events, config);

    EXPECT_EQ(removedCount, 0);
    EXPECT_EQ(events.size(), originalSize);
}

TEST_F(MouseMovementOptimizerTest, EmptyEventList)
{
    std::vector<std::unique_ptr<Event>> events;

    MouseMovementOptimizer::OptimizationConfig config;

    size_t removedCount =
        MouseMovementOptimizer::optimizeEvents(events, config);

    EXPECT_EQ(removedCount, 0);
    EXPECT_EQ(events.size(), 0);
}

TEST_F(MouseMovementOptimizerTest, OnlyNonMouseEvents)
{
    std::vector<std::unique_ptr<Event>> events;

    // Add only keyboard events
    KeyboardEventData keyData;
    keyData.keyCode = 65; // 'A'
    keyData.keyName = "A";

    events.push_back(std::make_unique<Event>(EventType::KeyPress, keyData));
    events.push_back(std::make_unique<Event>(EventType::KeyRelease, keyData));

    MouseMovementOptimizer::OptimizationConfig config;

    size_t originalSize = events.size();
    size_t removedCount =
        MouseMovementOptimizer::optimizeEvents(events, config);

    EXPECT_EQ(removedCount, 0);
    EXPECT_EQ(events.size(), originalSize);
}
