#pragma once

#include <QWidget>
#include "../core/Event.hpp"
#include <vector>
#include <memory>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

namespace Ui
{
class RecordingWidget;
}

namespace MouseRecorder::Application
{
class MouseRecorderApp;
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
    explicit RecordingWidget(
      Application::MouseRecorderApp& app, QWidget* parent = nullptr
    );
    ~RecordingWidget();

  public slots:
    void updateStatistics();
    void updateStatistics(
      size_t totalEvents, size_t mouseEvents, size_t keyboardEvents
    );
    void addEvent(const Core::Event* event);
    void clearEvents();
    void setEvents(const std::vector<std::unique_ptr<Core::Event>>& events);

  public:
    // Methods to update button states programmatically (for shortcuts)
    void setRecordingState(bool isRecording);
    void startRecordingUI();
    void stopRecordingUI();

  signals:
    void recordingStarted();
    void recordingStopped();
    void exportEventsRequested(
      const std::vector<std::unique_ptr<Core::Event>>& events
    );

  private slots:
    void onStartRecording();
    void onStopRecording();
    void onClearEvents();
    void onExportEvents();

  private:
    void setupUI();
    void updateRecordingTime();
    void loadConfigurationSettings();
    void saveConfigurationSettings();

  private slots:
    void onOptimizeMovementChanged(bool enabled);
    void onMovementThresholdChanged(int value);

  private:
    Ui::RecordingWidget* ui;
    Application::MouseRecorderApp& m_app;
    QTimer* m_recordingTimer;
    int m_recordingSeconds{0};

    // Store references to displayed events for export
    std::vector<const Core::Event*> m_displayedEvents;
};

} // namespace MouseRecorder::GUI
