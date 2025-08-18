#include <gtest/gtest.h>
#include <QApplication>
#include <QTemporaryFile>
#include <QTemporaryDir>

#include "core/IConfiguration.hpp"
#include "core/QtConfiguration.hpp"

using namespace MouseRecorder::Core;

class ConfigurationPersistenceTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create temporary configuration file
        m_tempDir = std::make_unique<QTemporaryDir>();
        ASSERT_TRUE(m_tempDir->isValid());

        m_configPath = m_tempDir->path() + "/test_config.conf";

        // Create configuration instance directly
        m_config = std::make_unique<QtConfiguration>();
    }

    void TearDown() override
    {
        m_config.reset();
        m_tempDir.reset();
    }

    std::unique_ptr<QTemporaryDir> m_tempDir;
    QString m_configPath;
    std::unique_ptr<QtConfiguration> m_config;
};

TEST_F(ConfigurationPersistenceTest, BasicConfigurationLoad)
{
    // Test that the configuration can load and store basic values

    // Set some known values in configuration
    m_config->setBool(ConfigKeys::CAPTURE_MOUSE_EVENTS, false);
    m_config->setInt(ConfigKeys::MOUSE_MOVEMENT_THRESHOLD, 10);
    m_config->setDouble(ConfigKeys::DEFAULT_PLAYBACK_SPEED, 2.5);
    m_config->setString(ConfigKeys::LOG_LEVEL, "debug");

    // Verify the values are stored correctly
    EXPECT_FALSE(m_config->getBool(ConfigKeys::CAPTURE_MOUSE_EVENTS));
    EXPECT_EQ(10, m_config->getInt(ConfigKeys::MOUSE_MOVEMENT_THRESHOLD));
    EXPECT_DOUBLE_EQ(2.5,
                     m_config->getDouble(ConfigKeys::DEFAULT_PLAYBACK_SPEED));
    EXPECT_STREQ("debug", m_config->getString(ConfigKeys::LOG_LEVEL).c_str());
}

TEST_F(ConfigurationPersistenceTest, ConfigurationSaveAndReload)
{
    // Test the complete save/reload cycle

    // Set test values
    const bool testMouseCapture = false;
    const bool testKeyboardCapture = true;
    const int testThreshold = 15;
    const double testSpeed = 3.0;
    const std::string testLogLevel = "error";
    const bool testLogToFile = true;
    const std::string testLogPath = "test_log.log";

    m_config->setBool(ConfigKeys::CAPTURE_MOUSE_EVENTS, testMouseCapture);
    m_config->setBool(ConfigKeys::CAPTURE_KEYBOARD_EVENTS, testKeyboardCapture);
    m_config->setInt(ConfigKeys::MOUSE_MOVEMENT_THRESHOLD, testThreshold);
    m_config->setDouble(ConfigKeys::DEFAULT_PLAYBACK_SPEED, testSpeed);
    m_config->setString(ConfigKeys::LOG_LEVEL, testLogLevel);
    m_config->setBool(ConfigKeys::LOG_TO_FILE, testLogToFile);
    m_config->setString(ConfigKeys::LOG_FILE_PATH, testLogPath);

    // Save configuration to file
    ASSERT_TRUE(m_config->saveToFile(m_configPath.toStdString()));

    // Create a new configuration instance and load from the same file
    auto newConfig = std::make_unique<QtConfiguration>();
    ASSERT_TRUE(newConfig->loadFromFile(m_configPath.toStdString()));

    // Verify values were persisted
    EXPECT_EQ(testMouseCapture,
              newConfig->getBool(ConfigKeys::CAPTURE_MOUSE_EVENTS));
    EXPECT_EQ(testKeyboardCapture,
              newConfig->getBool(ConfigKeys::CAPTURE_KEYBOARD_EVENTS));
    EXPECT_EQ(testThreshold,
              newConfig->getInt(ConfigKeys::MOUSE_MOVEMENT_THRESHOLD));
    EXPECT_DOUBLE_EQ(testSpeed,
                     newConfig->getDouble(ConfigKeys::DEFAULT_PLAYBACK_SPEED));
    EXPECT_STREQ(testLogLevel.c_str(),
                 newConfig->getString(ConfigKeys::LOG_LEVEL).c_str());
    EXPECT_EQ(testLogToFile, newConfig->getBool(ConfigKeys::LOG_TO_FILE));
    EXPECT_STREQ(testLogPath.c_str(),
                 newConfig->getString(ConfigKeys::LOG_FILE_PATH).c_str());
}

TEST_F(ConfigurationPersistenceTest, ConfigurationKeysExist)
{
    // Test that all required configuration keys can be set and retrieved

    // Test recording settings
    m_config->setBool(ConfigKeys::CAPTURE_MOUSE_EVENTS, true);
    m_config->setBool(ConfigKeys::CAPTURE_KEYBOARD_EVENTS, true);
    m_config->setBool(ConfigKeys::OPTIMIZE_MOUSE_MOVEMENTS, true);
    m_config->setInt(ConfigKeys::MOUSE_MOVEMENT_THRESHOLD, 5);

    // Test playback settings
    m_config->setDouble(ConfigKeys::DEFAULT_PLAYBACK_SPEED, 1.0);
    m_config->setBool(ConfigKeys::LOOP_PLAYBACK, false);
    m_config->setBool(ConfigKeys::SHOW_PLAYBACK_CURSOR, true);

    // Test shortcuts
    m_config->setString(ConfigKeys::SHORTCUT_START_RECORDING, "Ctrl+R");
    m_config->setString(ConfigKeys::SHORTCUT_STOP_RECORDING, "Ctrl+Shift+R");
    m_config->setString(ConfigKeys::SHORTCUT_START_PLAYBACK, "Ctrl+P");
    m_config->setString(ConfigKeys::SHORTCUT_STOP_PLAYBACK, "Ctrl+Shift+P");

    // Test logging settings
    m_config->setString(ConfigKeys::LOG_LEVEL, "info");
    m_config->setBool(ConfigKeys::LOG_TO_FILE, false);
    m_config->setString(ConfigKeys::LOG_FILE_PATH, "mouserecorder.log");

    // Save and verify no errors
    EXPECT_TRUE(m_config->saveToFile(m_configPath.toStdString()));
    EXPECT_STREQ("", m_config->getLastError().c_str());
}

TEST_F(ConfigurationPersistenceTest, ConfigurationFileExists)
{
    // Test that configuration files are actually created

    // Set some values and save
    m_config->setBool(ConfigKeys::CAPTURE_MOUSE_EVENTS, true);
    m_config->setInt(ConfigKeys::MOUSE_MOVEMENT_THRESHOLD, 5);

    EXPECT_TRUE(m_config->saveToFile(m_configPath.toStdString()));

    // Verify file was created
    EXPECT_TRUE(QFile::exists(m_configPath));
}
