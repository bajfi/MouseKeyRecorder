// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <QtTest/QtTest>
#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QSignalSpy>
#include <QTimer>
#include <memory>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <string>
#include "gui/MainWindow.hpp"
#include "application/MouseRecorderApp.hpp"

using namespace MouseRecorder::GUI;
using namespace MouseRecorder::Application;

class TestSystemTrayFunctionality : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testSystemTrayAvailable();
    void testTrayIconCreation();
    void testTrayMenuCreation();
    void testMinimizeToTray();
    void testRestoreFromTray();
    void testAutoMinimizeOnRecording();
    void testAutoRestoreOnRecordingStop();
    void testConfigurationToggle();
    void testTrayIconDoubleClick();
    void testContextMenuActions();

  private:
    QApplication* m_app{nullptr};
    std::unique_ptr<MouseRecorderApp> m_mouseApp;
    MainWindow* m_mainWindow{nullptr};

    void simulateRecordingStart();
    void simulateRecordingStop();
    void waitForWindowStateChange(int timeoutMs = 1000);
};

void TestSystemTrayFunctionality::initTestCase()
{
    // Check if we're in a CI environment on Windows - skip all GUI tests
#ifdef _WIN32
    char* ciEnv = nullptr;
    char* githubActions = nullptr;

    _dupenv_s(&ciEnv, nullptr, "CI");
    _dupenv_s(&githubActions, nullptr, "GITHUB_ACTIONS");

    bool isCI = (ciEnv && std::string(ciEnv) == "true") ||
                (githubActions && std::string(githubActions) == "true");

    // Clean up allocated memory
    if (ciEnv)
        free(ciEnv);
    if (githubActions)
        free(githubActions);

    // Skip all SystemTray tests in Windows CI
    if (isCI)
    {
        QSKIP("Skipping SystemTray tests in Windows CI environment. "
              "GUI tests timeout and are unreliable in Windows CI.");
    }
#endif

    // QApplication is required for GUI tests
    if (!QApplication::instance())
    {
        int argc = 0;
        char* argv[] = {nullptr};
        m_app = new QApplication(argc, argv);
    }
}

void TestSystemTrayFunctionality::cleanupTestCase()
{
    if (m_app && m_app->instance() == m_app)
    {
        delete m_app;
        m_app = nullptr;
    }
}

void TestSystemTrayFunctionality::init()
{
    // Create fresh application and window for each test
    m_mouseApp = std::make_unique<MouseRecorderApp>();
    QVERIFY(m_mouseApp->initialize());

    m_mainWindow = new MainWindow(*m_mouseApp);
    QVERIFY(m_mainWindow != nullptr);

    // Show window for tests
    m_mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow));
}

void TestSystemTrayFunctionality::cleanup()
{
    if (m_mainWindow)
    {
        m_mainWindow->close();
        delete m_mainWindow;
        m_mainWindow = nullptr;
    }

    m_mouseApp.reset();
}

void TestSystemTrayFunctionality::testSystemTrayAvailable()
{
    // This test may fail in headless environments, so make it conditional
    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        QSKIP("System tray not available in this environment");
    }

    QVERIFY(QSystemTrayIcon::isSystemTrayAvailable());
}

void TestSystemTrayFunctionality::testTrayIconCreation()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        QSKIP("System tray not available in this environment");
    }

    // Find the tray icon (it's a private member, so we test indirectly)
    QList<QSystemTrayIcon*> trayIcons =
        m_mainWindow->findChildren<QSystemTrayIcon*>();
    QVERIFY(trayIcons.size() > 0);

    QSystemTrayIcon* trayIcon = trayIcons.first();
    QVERIFY(trayIcon != nullptr);
    QVERIFY(trayIcon->isVisible());
    QCOMPARE(trayIcon->toolTip(), QString("MouseRecorder"));
}

void TestSystemTrayFunctionality::testTrayMenuCreation()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        QSKIP("System tray not available in this environment");
    }

    QList<QSystemTrayIcon*> trayIcons =
        m_mainWindow->findChildren<QSystemTrayIcon*>();
    QVERIFY(trayIcons.size() > 0);

    QSystemTrayIcon* trayIcon = trayIcons.first();
    QMenu* contextMenu = trayIcon->contextMenu();
    QVERIFY(contextMenu != nullptr);

    QList<QAction*> actions = contextMenu->actions();
    QVERIFY(actions.size() >= 2); // Show/Hide + Exit at minimum

    // Check for Show/Hide action
    bool hasShowHide = false;
    bool hasExit = false;

    for (QAction* action : actions)
    {
        if (action->text().contains("Show") || action->text().contains("Hide"))
        {
            hasShowHide = true;
        }
        if (action->text().contains("Exit"))
        {
            hasExit = true;
        }
    }

    QVERIFY(hasShowHide);
    QVERIFY(hasExit);
}

void TestSystemTrayFunctionality::testMinimizeToTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        QSKIP("System tray not available in this environment");
    }

    // Window should be visible initially
    QVERIFY(m_mainWindow->isVisible());

    // Find the minimize method via QMetaObject
    bool success = QMetaObject::invokeMethod(m_mainWindow, "minimizeToTray");
    QVERIFY(success);

    waitForWindowStateChange();

    // Window should be hidden after minimize to tray
    QVERIFY(!m_mainWindow->isVisible());
}

void TestSystemTrayFunctionality::testRestoreFromTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        QSKIP("System tray not available in this environment");
    }

    // First minimize to tray
    QMetaObject::invokeMethod(m_mainWindow, "minimizeToTray");
    waitForWindowStateChange();
    QVERIFY(!m_mainWindow->isVisible());

    // Then restore
    bool success = QMetaObject::invokeMethod(m_mainWindow, "restoreFromTray");
    QVERIFY(success);

    waitForWindowStateChange();

    // Window should be visible again
    QVERIFY(m_mainWindow->isVisible());
}

void TestSystemTrayFunctionality::testAutoMinimizeOnRecording()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        QSKIP("System tray not available in this environment");
    }

    // Ensure auto-minimize is enabled
    m_mouseApp->getConfiguration().setBool("ui.auto_minimize_on_record", true);

    // Window should be visible initially
    QVERIFY(m_mainWindow->isVisible());

    // Simulate recording start
    simulateRecordingStart();

    // Wait a bit longer for the timer delay in onRecordingStarted
    QTest::qWait(600);

    // Window should be hidden after recording starts
    QVERIFY(!m_mainWindow->isVisible());

    // Stop recording to clean up properly
    simulateRecordingStop();
    QTest::qWait(300); // Wait for restore timer
}

void TestSystemTrayFunctionality::testAutoRestoreOnRecordingStop()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        QSKIP("System tray not available in this environment");
    }

    // Ensure auto-minimize is enabled
    m_mouseApp->getConfiguration().setBool("ui.auto_minimize_on_record", true);

    // Start with recording and auto-minimize
    simulateRecordingStart();
    QTest::qWait(600);
    QVERIFY(!m_mainWindow->isVisible());

    // Stop recording
    simulateRecordingStop();
    QTest::qWait(300); // Wait for restore timer

    // Window should be visible again
    QVERIFY(m_mainWindow->isVisible());
}

void TestSystemTrayFunctionality::testConfigurationToggle()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        QSKIP("System tray not available in this environment");
    }

    // Disable auto-minimize
    m_mouseApp->getConfiguration().setBool("ui.auto_minimize_on_record", false);

    // Window should be visible initially
    QVERIFY(m_mainWindow->isVisible());

    // Simulate recording start
    simulateRecordingStart();
    QTest::qWait(600);

    // Window should still be visible (auto-minimize disabled)
    QVERIFY(m_mainWindow->isVisible());
}

void TestSystemTrayFunctionality::testTrayIconDoubleClick()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        QSKIP("System tray not available in this environment");
    }

    QList<QSystemTrayIcon*> trayIcons =
        m_mainWindow->findChildren<QSystemTrayIcon*>();
    QVERIFY(trayIcons.size() > 0);

    QSystemTrayIcon* trayIcon = trayIcons.first();

    // First minimize
    QMetaObject::invokeMethod(m_mainWindow, "minimizeToTray");
    waitForWindowStateChange();
    QVERIFY(!m_mainWindow->isVisible());

    // Simulate double-click on tray icon
    emit trayIcon->activated(QSystemTrayIcon::DoubleClick);
    waitForWindowStateChange();

    // Window should be restored
    QVERIFY(m_mainWindow->isVisible());
}

void TestSystemTrayFunctionality::testContextMenuActions()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        QSKIP("System tray not available in this environment");
    }

    QList<QSystemTrayIcon*> trayIcons =
        m_mainWindow->findChildren<QSystemTrayIcon*>();
    QVERIFY(trayIcons.size() > 0);

    QSystemTrayIcon* trayIcon = trayIcons.first();
    QMenu* contextMenu = trayIcon->contextMenu();
    QVERIFY(contextMenu != nullptr);

    QList<QAction*> actions = contextMenu->actions();

    // Find Show/Hide action and test it
    QAction* showHideAction = nullptr;
    for (QAction* action : actions)
    {
        if (action->text().contains("Show") || action->text().contains("Hide"))
        {
            showHideAction = action;
            break;
        }
    }

    QVERIFY(showHideAction != nullptr);

    // Test triggering the action
    QVERIFY(m_mainWindow->isVisible());

    showHideAction->trigger();
    waitForWindowStateChange();

    // Window should be hidden
    QVERIFY(!m_mainWindow->isVisible());

    // Trigger again to restore
    showHideAction->trigger();
    waitForWindowStateChange();

    // Window should be visible again
    QVERIFY(m_mainWindow->isVisible());
}

void TestSystemTrayFunctionality::simulateRecordingStart()
{
    // Call onRecordingStarted directly to simulate the recording state
    QMetaObject::invokeMethod(m_mainWindow, "onRecordingStarted");
}

void TestSystemTrayFunctionality::simulateRecordingStop()
{
    // Call onRecordingStopped directly to simulate the recording state
    QMetaObject::invokeMethod(m_mainWindow, "onRecordingStopped");
}

void TestSystemTrayFunctionality::waitForWindowStateChange(int timeoutMs)
{
    // Give Qt time to process window state changes
    QTest::qWait(timeoutMs);
    QApplication::processEvents();
}

QTEST_APPLESS_MAIN(TestSystemTrayFunctionality)
#include "test_SystemTrayFunctionality.moc"
