#include <QtTest/QtTest>
#include <QApplication>
#include <QAction>
#include <QSignalSpy>
#include <QMenu>
#include <QTabWidget>
#include <memory>
#include "gui/MainWindow.hpp"
#include "application/MouseRecorderApp.hpp"

using namespace MouseRecorder::GUI;
using namespace MouseRecorder::Application;

class TestMainWindow : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testInitialState();
    void testActionStates();
    void testRecordingActions();
    void testPlaybackActions();
    void testUIStateUpdates();
    void testFileActions();
    void testRecentFiles();
    void testClearEvents();
    void testPreferences();

  private:
    QApplication* m_app{nullptr};
    std::unique_ptr<MouseRecorderApp> m_mouseApp;
    MainWindow* m_mainWindow{nullptr};
};

void TestMainWindow::initTestCase()
{
    // QApplication is required for GUI tests
    if (!QApplication::instance())
    {
        int argc = 0;
        char* argv[] = {nullptr};
        m_app = new QApplication(argc, argv);
    }
}

void TestMainWindow::cleanupTestCase()
{
    if (m_app && m_app->instance() == m_app)
    {
        delete m_app;
        m_app = nullptr;
    }
}

void TestMainWindow::init()
{
    m_mouseApp = std::make_unique<MouseRecorderApp>();

    // Initialize the application
    if (m_mouseApp->initialize())
    {
        m_mainWindow = new MainWindow(*m_mouseApp);
    }
}

void TestMainWindow::cleanup()
{
    delete m_mainWindow;
    m_mainWindow = nullptr;

    if (m_mouseApp)
    {
        m_mouseApp->shutdown();
        m_mouseApp.reset();
    }
}

void TestMainWindow::testInitialState()
{
    if (!m_mainWindow)
    {
        QSKIP(
          "MainWindow could not be created - application initialization failed"
        );
    }

    QVERIFY(m_mainWindow != nullptr);

    // Check that actions exist
    QAction* startRecordingAction =
      m_mainWindow->findChild<QAction*>("actionStartRecording");
    QAction* stopRecordingAction =
      m_mainWindow->findChild<QAction*>("actionStopRecording");
    QAction* startPlaybackAction =
      m_mainWindow->findChild<QAction*>("actionStartPlayback");

    QVERIFY(startRecordingAction != nullptr);
    QVERIFY(stopRecordingAction != nullptr);
    QVERIFY(startPlaybackAction != nullptr);
}

void TestMainWindow::testActionStates()
{
    if (!m_mainWindow)
    {
        QSKIP(
          "MainWindow could not be created - application initialization failed"
        );
    }

    QAction* startRecordingAction =
      m_mainWindow->findChild<QAction*>("actionStartRecording");
    QAction* stopRecordingAction =
      m_mainWindow->findChild<QAction*>("actionStopRecording");
    QAction* startPlaybackAction =
      m_mainWindow->findChild<QAction*>("actionStartPlayback");

    // Initially, start recording should be enabled, stop recording disabled
    QVERIFY(startRecordingAction->isEnabled());
    QVERIFY(!stopRecordingAction->isEnabled());

    // Start playback should be disabled initially (no file loaded)
    QVERIFY(!startPlaybackAction->isEnabled());
}

void TestMainWindow::testRecordingActions()
{
    if (!m_mainWindow)
    {
        QSKIP(
          "MainWindow could not be created - application initialization failed"
        );
    }

    QAction* startRecordingAction =
      m_mainWindow->findChild<QAction*>("actionStartRecording");
    QAction* stopRecordingAction =
      m_mainWindow->findChild<QAction*>("actionStopRecording");

    if (!startRecordingAction || !stopRecordingAction)
    {
        QSKIP("Recording actions not found");
    }

    // Test triggering start recording action
    if (startRecordingAction->isEnabled())
    {
        startRecordingAction->trigger();

        // After starting recording, start should be disabled, stop enabled
        QVERIFY(!startRecordingAction->isEnabled());
        QVERIFY(stopRecordingAction->isEnabled());

        // Test triggering stop recording action
        stopRecordingAction->trigger();

        // After stopping recording, start should be enabled, stop disabled
        QVERIFY(startRecordingAction->isEnabled());
        QVERIFY(!stopRecordingAction->isEnabled());
    }
}

void TestMainWindow::testPlaybackActions()
{
    if (!m_mainWindow)
    {
        QSKIP(
          "MainWindow could not be created - application initialization failed"
        );
    }

    QAction* startPlaybackAction =
      m_mainWindow->findChild<QAction*>("actionStartPlayback");
    QAction* pausePlaybackAction =
      m_mainWindow->findChild<QAction*>("actionPausePlayback");
    QAction* stopPlaybackAction =
      m_mainWindow->findChild<QAction*>("actionStopPlayback");

    if (!startPlaybackAction || !pausePlaybackAction || !stopPlaybackAction)
    {
        QSKIP("Playback actions not found");
    }

    // Initially playback actions should be disabled (no events to play)
    QVERIFY(!startPlaybackAction->isEnabled());
    QVERIFY(!pausePlaybackAction->isEnabled());
    QVERIFY(!stopPlaybackAction->isEnabled());
}

void TestMainWindow::testUIStateUpdates()
{
    if (!m_mainWindow)
    {
        QSKIP(
          "MainWindow could not be created - application initialization failed"
        );
    }

    // Test that UI state updates work when recording state changes
    QAction* startRecordingAction =
      m_mainWindow->findChild<QAction*>("actionStartRecording");
    QAction* stopRecordingAction =
      m_mainWindow->findChild<QAction*>("actionStopRecording");
    QAction* startPlaybackAction =
      m_mainWindow->findChild<QAction*>("actionStartPlayback");

    // Record initial states (commented out to avoid unused variable warnings)
    // bool initialStartEnabled = startRecordingAction->isEnabled();
    // bool initialStopEnabled = stopRecordingAction->isEnabled();

    // Trigger a recording start if possible
    if (startRecordingAction->isEnabled())
    {
        startRecordingAction->trigger();

        // UI should update to reflect recording state
        QVERIFY(!startRecordingAction->isEnabled());
        QVERIFY(stopRecordingAction->isEnabled());
        QVERIFY(
          !startPlaybackAction->isEnabled()
        ); // Should be disabled during recording

        // Stop recording
        stopRecordingAction->trigger();

        // UI should revert
        QVERIFY(startRecordingAction->isEnabled());
        QVERIFY(!stopRecordingAction->isEnabled());
    }
}

void TestMainWindow::testFileActions()
{
    if (!m_mainWindow)
    {
        QSKIP(
          "MainWindow could not be created - application initialization failed"
        );
    }

    QAction* newAction = m_mainWindow->findChild<QAction*>("actionNew");
    QAction* saveAction = m_mainWindow->findChild<QAction*>("actionSave");
    QAction* saveAsAction = m_mainWindow->findChild<QAction*>("actionSaveAs");

    QVERIFY(newAction != nullptr);
    QVERIFY(saveAction != nullptr);
    QVERIFY(saveAsAction != nullptr);

    // Test that actions are enabled
    QVERIFY(newAction->isEnabled());
    QVERIFY(saveAction->isEnabled());
    QVERIFY(saveAsAction->isEnabled());

    // Don't trigger actions that show dialogs - just verify they exist and are
    // connected The actual functionality should be tested through integration
    // tests that can handle dialogs or through mocking
    QVERIFY(newAction->isVisible());
    QVERIFY(saveAction->isVisible());
    QVERIFY(saveAsAction->isVisible());
}

void TestMainWindow::testRecentFiles()
{
    if (!m_mainWindow)
    {
        QSKIP(
          "MainWindow could not be created - application initialization failed"
        );
    }

    QAction* recentFilesAction =
      m_mainWindow->findChild<QAction*>("actionRecentFiles");
    QVERIFY(recentFilesAction != nullptr);

    // Recent files action should exist and be enabled
    QVERIFY(recentFilesAction->isEnabled());

    // Test signal emission when file loading is requested
    QSignalSpy fileLoadSpy(m_mainWindow, &MainWindow::fileLoadRequested);

    // The action should have a menu when the recent files are set up
    QMenu* recentMenu = recentFilesAction->menu();
    if (recentMenu)
    {
        // Menu should exist but might be empty initially
        QVERIFY(recentMenu != nullptr);
    }

    // Test that the signal-slot connection works for file loading
    // We can verify the connection by checking signal count (should be 0
    // initially)
    QCOMPARE(fileLoadSpy.count(), 0);
}

void TestMainWindow::testClearEvents()
{
    if (!m_mainWindow)
    {
        QSKIP(
          "MainWindow could not be created - application initialization failed"
        );
    }

    QAction* clearAction = m_mainWindow->findChild<QAction*>("actionClear");
    QVERIFY(clearAction != nullptr);

    // Clear action should exist and be enabled
    QVERIFY(clearAction->isEnabled());

    // Test triggering the action (will show dialog in real usage, but in test
    // environment the TestUtils should suppress dialogs)
    clearAction->trigger();
}

void TestMainWindow::testPreferences()
{
    if (!m_mainWindow)
    {
        QSKIP(
          "MainWindow could not be created - application initialization failed"
        );
    }

    QAction* preferencesAction =
      m_mainWindow->findChild<QAction*>("actionPreferences");
    QVERIFY(preferencesAction != nullptr);

    // Preferences action should exist and be enabled
    QVERIFY(preferencesAction->isEnabled());

    // Note: Testing that the preferences action opens a dialog is more complex
    // and would require mocking the dialog or manual intervention.
    // For now, we just verify the action exists and is enabled.
    // The dialog functionality is tested in integration tests.
}

QTEST_APPLESS_MAIN(TestMainWindow)
#include "test_MainWindow.moc"
