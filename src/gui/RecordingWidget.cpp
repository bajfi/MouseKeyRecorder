#include "RecordingWidget.hpp"
#include "ui_RecordingWidget.h"
#include <QTimer>

namespace MouseRecorder::GUI
{

RecordingWidget::RecordingWidget(QWidget* parent)
  : QWidget(parent),
    ui(new Ui::RecordingWidget),
    m_recordingTimer(new QTimer(this))
{
    ui->setupUi(this);
    setupUI();
}

RecordingWidget::~RecordingWidget()
{
    delete ui;
}

void RecordingWidget::setupUI()
{
    // Connect signals
    connect(
      ui->startRecordingButton,
      &QPushButton::clicked,
      this,
      &RecordingWidget::onStartRecording
    );
    connect(
      ui->stopRecordingButton,
      &QPushButton::clicked,
      this,
      &RecordingWidget::onStopRecording
    );
    connect(
      ui->clearEventsButton,
      &QPushButton::clicked,
      this,
      &RecordingWidget::onClearEvents
    );
    connect(
      ui->exportEventsButton,
      &QPushButton::clicked,
      this,
      &RecordingWidget::onExportEvents
    );

    // Setup timer
    connect(
      m_recordingTimer,
      &QTimer::timeout,
      this,
      &RecordingWidget::updateRecordingTime
    );
    m_recordingTimer->setInterval(1000); // Update every second

    // Initialize table headers
    ui->eventsTableWidget->setColumnCount(3);
    QStringList headers = {"Time", "Type", "Details"};
    ui->eventsTableWidget->setHorizontalHeaderLabels(headers);
}

void RecordingWidget::updateStatistics()
{
    // This is a placeholder implementation when called without parameters
    // The overloaded version should be called instead
}

void RecordingWidget::updateStatistics(
  size_t totalEvents, size_t mouseEvents, size_t keyboardEvents
)
{
    ui->eventsCountValue->setText(QString::number(totalEvents));
    ui->mouseEventsValue->setText(QString::number(mouseEvents));
    ui->keyboardEventsValue->setText(QString::number(keyboardEvents));
}

void RecordingWidget::onStartRecording()
{
    ui->startRecordingButton->setEnabled(false);
    ui->stopRecordingButton->setEnabled(true);

    m_recordingSeconds = 0;
    m_recordingTimer->start();

    emit recordingStarted();
}

void RecordingWidget::onStopRecording()
{
    ui->startRecordingButton->setEnabled(true);
    ui->stopRecordingButton->setEnabled(false);

    m_recordingTimer->stop();

    emit recordingStopped();

    // TODO: Stop actual recording
}

void RecordingWidget::onClearEvents()
{
    ui->eventsTableWidget->setRowCount(0);
    ui->eventsCountValue->setText("0");
    ui->mouseEventsValue->setText("0");
    ui->keyboardEventsValue->setText("0");
    ui->recordingDurationValue->setText("00:00:00");

    // TODO: Clear actual event data
}

void RecordingWidget::onExportEvents()
{
    // TODO: Implement export functionality
}

void RecordingWidget::updateRecordingTime()
{
    m_recordingSeconds++;

    int hours = m_recordingSeconds / 3600;
    int minutes = (m_recordingSeconds % 3600) / 60;
    int seconds = m_recordingSeconds % 60;

    QString timeStr = QString("%1:%2:%3")
                        .arg(hours, 2, 10, QChar('0'))
                        .arg(minutes, 2, 10, QChar('0'))
                        .arg(seconds, 2, 10, QChar('0'));

    ui->recordingTimeLabel->setText(timeStr);
    ui->recordingDurationValue->setText(timeStr);
}

} // namespace MouseRecorder::GUI

