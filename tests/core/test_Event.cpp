// Copyright (c) 2025 JackLee
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <gtest/gtest.h>
#include "core/Event.hpp"

using namespace MouseRecorder::Core;

class EventTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Test setup
    }

    void TearDown() override
    {
        // Test cleanup
    }
};

TEST_F(EventTest, CreateMouseMoveEvent)
{
    Point position{100, 200};
    auto event =
        EventFactory::createMouseMoveEvent(position, KeyModifier::Ctrl);

    ASSERT_NE(event, nullptr);
    EXPECT_EQ(event->getType(), EventType::MouseMove);
    EXPECT_TRUE(event->isMouseEvent());
    EXPECT_FALSE(event->isKeyboardEvent());

    const auto* mouseData = event->getMouseData();
    ASSERT_NE(mouseData, nullptr);
    EXPECT_EQ(mouseData->position.x, 100);
    EXPECT_EQ(mouseData->position.y, 200);
    EXPECT_EQ(mouseData->modifiers, KeyModifier::Ctrl);
}

TEST_F(EventTest, CreateMouseClickEvent)
{
    Point position{150, 250};
    auto event = EventFactory::createMouseClickEvent(
        position, MouseButton::Right, KeyModifier::Shift);

    ASSERT_NE(event, nullptr);
    EXPECT_EQ(event->getType(), EventType::MouseClick);
    EXPECT_TRUE(event->isMouseEvent());

    const auto* mouseData = event->getMouseData();
    ASSERT_NE(mouseData, nullptr);
    EXPECT_EQ(mouseData->position.x, 150);
    EXPECT_EQ(mouseData->position.y, 250);
    EXPECT_EQ(mouseData->button, MouseButton::Right);
    EXPECT_EQ(mouseData->modifiers, KeyModifier::Shift);
}

TEST_F(EventTest, CreateKeyPressEvent)
{
    auto event = EventFactory::createKeyPressEvent(65, "A", KeyModifier::Alt);

    ASSERT_NE(event, nullptr);
    EXPECT_EQ(event->getType(), EventType::KeyPress);
    EXPECT_FALSE(event->isMouseEvent());
    EXPECT_TRUE(event->isKeyboardEvent());

    const auto* keyData = event->getKeyboardData();
    ASSERT_NE(keyData, nullptr);
    EXPECT_EQ(keyData->keyCode, 65);
    EXPECT_EQ(keyData->keyName, "A");
    EXPECT_EQ(keyData->modifiers, KeyModifier::Alt);
}

TEST_F(EventTest, TimestampConsistency)
{
    auto event1 = EventFactory::createMouseMoveEvent({0, 0});
    auto event2 = EventFactory::createMouseMoveEvent({10, 10});

    // Second event should have later or equal timestamp
    EXPECT_GE(event2->getTimestampMs(), event1->getTimestampMs());
}

TEST_F(EventTest, KeyModifierCombination)
{
    KeyModifier combined = KeyModifier::Ctrl | KeyModifier::Shift;
    auto event = EventFactory::createKeyPressEvent(65, "A", combined);

    const auto* keyData = event->getKeyboardData();
    ASSERT_NE(keyData, nullptr);

    // Check that both modifiers are present
    EXPECT_NE(static_cast<uint32_t>(keyData->modifiers & KeyModifier::Ctrl), 0);
    EXPECT_NE(static_cast<uint32_t>(keyData->modifiers & KeyModifier::Shift),
              0);
}

TEST_F(EventTest, EventToString)
{
    auto mouseEvent =
        EventFactory::createMouseClickEvent({100, 200}, MouseButton::Left);
    auto keyEvent = EventFactory::createKeyPressEvent(65, "A");

    std::string mouseStr = mouseEvent->toString();
    std::string keyStr = keyEvent->toString();

    EXPECT_FALSE(mouseStr.empty());
    EXPECT_FALSE(keyStr.empty());

    // Should contain event type
    EXPECT_NE(mouseStr.find("MouseClick"), std::string::npos);
    EXPECT_NE(keyStr.find("KeyPress"), std::string::npos);

    // Should contain position/key info
    EXPECT_NE(mouseStr.find("100,200"), std::string::npos);
    EXPECT_NE(keyStr.find("A"), std::string::npos);
}
