#pragma once

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QShortcut>
#include "application/MouseRecorderApp.hpp"
#include "core/Event.hpp"
#include "core/MouseMovementOptimizer.hpp"
#include <vector>
#include <memory>
#include <mutex>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QLabel;
class QMenu;
class QAction;
class QShortcut;
class QTimer;
QT_END_NAMESPACE

namespace Ui
{
class MainWindow;
}

namespace MouseRecorder::GUI
{
class RecordingWidget;
class PlaybackWidget;
} // namespace MouseRecorder::GUI

namespace MouseRecorder::GUI
{

/**
 * @brief Main application window
 *
 * This class represents the main window of the MouseRecorder application,
 * coordinating the various UI components and providing the main interface.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(Application::MouseRecorderApp& app,
                        QWidget* parent = nullptr);
    ~MainWindow();

  protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

  private slots:
    void onNewFile();
    void onSaveFile();
    void onSaveAsFile();
    void onRecentFiles();
    void onClear();
    void onPreferences();
    void onExit();
    void onAbout();
    void onAboutQt();

    void onStartRecording();
    void onStopRecording();
    void onStartPlayback();
    void onStopPlayback();
    void onExportEvents();

    // Widget signal handlers
    void onRecordingStarted();
    void onRecordingStopped();
    void onPlaybackStarted();
    void onPlaybackStopped();
    void onFileLoaded(const QString& filename);

    // System tray slots
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onShowHideAction();
    void onTrayExitAction();

  public slots:
    // Public methods for testing tray functionality
    void minimizeToTray();
    void restoreFromTray();

  signals:
    void fileLoadRequested(const QString& filename);

  private:
    void setupWidgets();
    void setupActions();
    void setupKeyboardShortcuts();
    void setupStatusBar();
    void setupSystemTray();
    void updateUI();
    void updateWindowTitle();
    void updateRecordingStatistics();
    void updateRecentFilesMenu();
    void addToRecentFiles(const QString& filename);
    void loadRecentFiles();
    void saveRecentFiles();
    bool saveEventsToFile(const QString& filename);

    bool shouldAutoMinimize() const;

    // Helper method for mouse movement optimization
    Core::MouseMovementOptimizer::OptimizationConfig
    getOptimizationConfigFromSettings() const;
    size_t applyMouseMovementOptimization(
        std::vector<std::unique_ptr<Core::Event>>& events) const;

    // Helper methods to suppress message boxes in test environments
    void showErrorMessage(const QString& title, const QString& message);
    void showWarningMessage(const QString& title, const QString& message);
    void showInfoMessage(const QString& title, const QString& message);

  private:
    Ui::MainWindow* ui;
    Application::MouseRecorderApp& m_app;
    QString m_currentFile;
    bool m_modified{false};
    QStringList m_recentFiles;
    QMenu* m_recentFilesMenu{nullptr};
    static constexpr int MaxRecentFiles = 10;

    // Custom widgets
    RecordingWidget* m_recordingWidget{nullptr};
    PlaybackWidget* m_playbackWidget{nullptr};

    // Event storage for recording session
    std::vector<std::unique_ptr<Core::Event>> m_recordedEvents;
    mutable std::mutex m_eventsMutex;

    // System tray components
    QSystemTrayIcon* m_trayIcon{nullptr};
    QMenu* m_trayMenu{nullptr};
    QAction* m_showHideAction{nullptr};
    QAction* m_exitAction{nullptr};
    bool m_wasVisibleBeforeMinimize{true};

    // Global shortcuts
    QShortcut* m_startRecordingShortcut{nullptr};
    QShortcut* m_stopRecordingShortcut{nullptr};
    QShortcut* m_startPlaybackShortcut{nullptr};
    QShortcut* m_stopPlaybackShortcut{nullptr};

    // Global shortcut monitoring
    QTimer* m_globalShortcutTimer{nullptr};
    void setupGlobalShortcuts();
    void checkGlobalShortcuts();
    bool isKeyPressed(int keyCode);

    // Shortcut states to prevent repeating
    bool m_startRecordingKeyPressed{false};
    bool m_stopRecordingKeyPressed{false};
};

} // namespace MouseRecorder::GUI
