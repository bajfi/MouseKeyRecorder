#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

namespace Ui
{
class RecordingWidget;
}

namespace MouseRecorder::GUI
{

/**
 * @brief Widget for recording controls and statistics
 */
class RecordingWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit RecordingWidget(QWidget* parent = nullptr);
    ~RecordingWidget();

  public slots:
    void updateStatistics();
    void updateStatistics(
      size_t totalEvents, size_t mouseEvents, size_t keyboardEvents
    );

  signals:
    void recordingStarted();
    void recordingStopped();

  private slots:
    void onStartRecording();
    void onStopRecording();
    void onClearEvents();
    void onExportEvents();

  private:
    void setupUI();
    void updateRecordingTime();

  private:
    Ui::RecordingWidget* ui;
    QTimer* m_recordingTimer;
    int m_recordingSeconds{0};
};

} // namespace MouseRecorder::GUI
