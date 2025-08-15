#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <memory>

#include "core/IConfiguration.hpp"
#include "core/QtConfiguration.hpp"

using namespace MouseRecorder::Core;

// Simple test just for QtConfiguration without GUI
TEST(SimpleConfigurationTest, BasicSetAndGet)
{
    // This test should run very fast
    auto config = std::make_unique<QtConfiguration>();

    // Test basic set/get operations
    config->setBool(ConfigKeys::CAPTURE_MOUSE_EVENTS, false);
    config->setInt(ConfigKeys::MOUSE_MOVEMENT_THRESHOLD, 10);
    config->setDouble(ConfigKeys::DEFAULT_PLAYBACK_SPEED, 2.5);
    config->setString(ConfigKeys::LOG_LEVEL, "debug");

    // Verify the values
    EXPECT_FALSE(config->getBool(ConfigKeys::CAPTURE_MOUSE_EVENTS));
    EXPECT_EQ(10, config->getInt(ConfigKeys::MOUSE_MOVEMENT_THRESHOLD));
    EXPECT_DOUBLE_EQ(
      2.5, config->getDouble(ConfigKeys::DEFAULT_PLAYBACK_SPEED)
    );
    EXPECT_STREQ("debug", config->getString(ConfigKeys::LOG_LEVEL).c_str());
}
