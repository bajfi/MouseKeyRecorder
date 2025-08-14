#pragma once

#include <QMainWindow>
#include "application/MouseRecorderApp.hpp"
#include "core/Event.hpp"
#include <vector>
#include <memory>
#include <mutex>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QLabel;
QT_END_NAMESPACE

namespace Ui
{
class MainWindow;
}

namespace MouseRecorder::GUI
{
class RecordingWidget;
class PlaybackWidget;
class ConfigurationWidget;
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
    explicit MainWindow(
      Application::MouseRecorderApp& app, QWidget* parent = nullptr
    );
    ~MainWindow();

  protected:
    void closeEvent(QCloseEvent* event) override;

  private slots:
    void onNewFile();
    void onOpenFile();
    void onSaveFile();
    void onSaveAsFile();
    void onExit();
    void onAbout();
    void onAboutQt();

    void onStartRecording();
    void onStopRecording();
    void onStartPlayback();
    void onPausePlayback();
    void onStopPlayback();
    void onExportEvents();

    // Widget signal handlers
    void onRecordingStarted();
    void onRecordingStopped();
    void onPlaybackStarted();
    void onPlaybackStopped();

  private:
    void setupWidgets();
    void setupActions();
    void setupStatusBar();
    void updateUI();
    void updateWindowTitle();
    void updateRecordingStatistics();

    // Helper methods to suppress message boxes in test environments
    void showErrorMessage(const QString& title, const QString& message);
    void showWarningMessage(const QString& title, const QString& message);
    void showInfoMessage(const QString& title, const QString& message);

  private:
    Ui::MainWindow* ui;
    Application::MouseRecorderApp& m_app;
    QString m_currentFile;
    bool m_modified{false};

    // Custom widgets
    RecordingWidget* m_recordingWidget{nullptr};
    PlaybackWidget* m_playbackWidget{nullptr};
    ConfigurationWidget* m_configurationWidget{nullptr};

    // Event storage for recording session
    std::vector<std::unique_ptr<Core::Event>> m_recordedEvents;
    mutable std::mutex m_eventsMutex;
};

} // namespace MouseRecorder::GUI
