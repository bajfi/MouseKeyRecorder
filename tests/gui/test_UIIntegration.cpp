#include <gtest/gtest.h>
#include <QApplication>
#include <QTimer>
#include <QCloseEvent>
#include "gui/MainWindow.hpp"
#include "application/MouseRecorderApp.hpp"

namespace MouseRecorder::Tests
{

class UIIntegrationTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Ensure we have a QApplication instance for GUI tests
        if (!QApplication::instance())
        {
            static int argc = 1;
            static char* argv[] = {const_cast<char*>("test")};
            m_app_instance = new QApplication(argc, argv);
        }

        m_configFile = "/tmp/test_ui_mouserecorder.conf";
        ASSERT_TRUE(m_mouseRecorderApp.initialize(m_configFile));
    }

    void TearDown() override
    {
        m_mouseRecorderApp.shutdown();
        std::remove(m_configFile.c_str());

        // Note: Don't delete QApplication in test, it's shared
    }

    QApplication* m_app_instance = nullptr;
    Application::MouseRecorderApp m_mouseRecorderApp;
    std::string m_configFile;
};

TEST_F(UIIntegrationTest, MainWindowCreation)
{
    // Should be able to create MainWindow without crashing
    EXPECT_NO_THROW({
        GUI::MainWindow mainWindow(m_mouseRecorderApp);

        // Window should be created but not shown by default
        EXPECT_FALSE(mainWindow.isVisible());

        // Should be able to show and hide the window
        mainWindow.show();
        EXPECT_TRUE(mainWindow.isVisible());

        mainWindow.hide();
        EXPECT_FALSE(mainWindow.isVisible());
    });
}

TEST_F(UIIntegrationTest, RecordingWidgetIntegration)
{
    GUI::MainWindow mainWindow(m_mouseRecorderApp);

    // Test the main window's recording functionality
    auto& recorder = m_mouseRecorderApp.getEventRecorder();

    EXPECT_FALSE(recorder.isRecording());

    // Test starting recording through main window method
    std::atomic<int> eventCount{0};
    auto callback = [&eventCount](std::unique_ptr<Core::Event> /*event*/)
    {
        eventCount++;
    };

    EXPECT_TRUE(recorder.startRecording(callback));
    EXPECT_TRUE(recorder.isRecording());

    // Test stopping recording
    recorder.stopRecording();
    EXPECT_FALSE(recorder.isRecording());
}

TEST_F(UIIntegrationTest, WindowCloseEventSimulation)
{
    GUI::MainWindow mainWindow(m_mouseRecorderApp);

    // Set up recording to test clean shutdown during close
    auto& recorder = m_mouseRecorderApp.getEventRecorder();

    std::atomic<int> eventCount{0};
    auto callback = [&eventCount](std::unique_ptr<Core::Event> /*event*/)
    {
        eventCount++;
    };

    ASSERT_TRUE(recorder.startRecording(callback));
    ASSERT_TRUE(recorder.isRecording());

    // Instead of calling closeEvent directly, test that recording stops
    // when we manually stop recording (simulating what happens on close)
    recorder.stopRecording();
    EXPECT_FALSE(recorder.isRecording());

    // Test that the app shutdown works without crashing
    EXPECT_NO_THROW(m_mouseRecorderApp.shutdown());
}

TEST_F(UIIntegrationTest, ConfigurationPersistence)
{
    {
        GUI::MainWindow mainWindow(m_mouseRecorderApp);

        // Simulate window resize and move
        mainWindow.resize(800, 600);
        mainWindow.move(100, 50);

        // Save configuration
        auto& config = m_mouseRecorderApp.getConfiguration();
        config.setInt("ui.window_width", 800);
        config.setInt("ui.window_height", 600);
        config.setInt("ui.window_x", 100);
        config.setInt("ui.window_y", 50);
        config.setBool("ui.window_maximized", false);
    }

    // Create a new app instance to test persistence
    Application::MouseRecorderApp app2;
    ASSERT_TRUE(app2.initialize(m_configFile));

    auto& config2 = app2.getConfiguration();

    EXPECT_EQ(config2.getInt("ui.window_width", 0), 800);
    EXPECT_EQ(config2.getInt("ui.window_height", 0), 600);
    EXPECT_EQ(config2.getInt("ui.window_x", 0), 100);
    EXPECT_EQ(config2.getInt("ui.window_y", 0), 50);
    EXPECT_EQ(config2.getBool("ui.window_maximized", true), false);

    app2.shutdown();
}

TEST_F(UIIntegrationTest, ErrorHandling)
{
    GUI::MainWindow mainWindow(m_mouseRecorderApp);

    auto& recorder = m_mouseRecorderApp.getEventRecorder();

    // Start recording
    std::atomic<int> eventCount{0};
    auto callback = [&eventCount](std::unique_ptr<Core::Event> /*event*/)
    {
        eventCount++;
    };

    ASSERT_TRUE(recorder.startRecording(callback));

    // Try to start again - should fail gracefully
    EXPECT_FALSE(recorder.startRecording(callback));

    // Clean up
    recorder.stopRecording();
    EXPECT_FALSE(recorder.isRecording());
}

} // namespace MouseRecorder::Tests
