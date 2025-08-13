#pragma once

#include <QWidget>

namespace Ui
{
class PlaybackWidget;
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
    explicit PlaybackWidget(QWidget* parent = nullptr);
    ~PlaybackWidget();

  signals:
    void playbackStarted();
    void playbackStopped();

  private slots:
    void onBrowseFile();
    void onReloadFile();
    void onPlay();
    void onPause();
    void onStop();
    void onSpeedChanged(int value);
    void onResetSpeed();
    void onProgressChanged(int value);

  private:
    void setupUI();
    void loadFile(const QString& fileName);
    void updateUI();
    void updateSpeed();

  private:
    Ui::PlaybackWidget* ui;
    QString m_currentFile;
    bool m_fileLoaded{false};
};

} // namespace MouseRecorder::GUI
