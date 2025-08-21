// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include <QWidget>
#include "core/IEventPlayer.hpp"
#include "core/EventTypes.hpp"

namespace Ui
{
class PlaybackWidget;
}

namespace MouseRecorder::Application
{
class MouseRecorderApp;
}

namespace MouseRecorder::GUI
{

/**
 * @brief Widget for playback controls and file management
 */
class PlaybackWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit PlaybackWidget(Application::MouseRecorderApp& app,
                            QWidget* parent = nullptr);
    ~PlaybackWidget();

    // Public method for testing
    void loadFile(const QString& fileName);

  signals:
    void playbackStarted();
    void playbackStopped();
    void playbackStateChanged(Core::PlaybackState state);
    void fileLoaded(const QString& filename);

  private slots:
    void onBrowseFile();
    void onReloadFile();
    void onPlay();
    void onStop();
    void onSpeedChanged(int value);
    void onResetSpeed();
    void onProgressChanged(int value);
    void onLoopToggled(bool enabled);
    void onLoopCountChanged(int count);
    void updatePlaybackProgress();

  public slots:
    void loadFileRequested(const QString& filename);

  private:
    void setupUI();
    void loadConfigurationSettings();
    void updateUI();
    void updateSpeed();
    void updatePlaybackStatus();
    void onPlaybackProgress(Core::PlaybackState state,
                            size_t current,
                            size_t total);
    void onEventPlayed(const Core::Event& event);

    // Helper methods for time display
    void updateTimeLabels(size_t currentEvent, size_t totalEvents);
    QString formatTime(std::chrono::milliseconds duration);

    // Helper method to show message boxes only when not in test environment
    void showErrorMessage(const QString& title, const QString& message);
    void showWarningMessage(const QString& title, const QString& message);

  private:
    Ui::PlaybackWidget* ui;
    Application::MouseRecorderApp& m_app;
    QString m_currentFile;
    bool m_fileLoaded{false};
    // Using unique_ptr to avoid Qt MOC registration issues with non-copyable
    // types
    std::unique_ptr<Core::EventVector> m_loadedEvents;
    QTimer* m_updateTimer{nullptr};
};

} // namespace MouseRecorder::GUI
