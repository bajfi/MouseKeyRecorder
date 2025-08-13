#include "RecordingWidget.hpp"
#include "ui_RecordingWidget.h"
#include <QTimer>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QDateTime>

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

    // Configure table appearance
    ui->eventsTableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->eventsTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->eventsTableWidget->setAlternatingRowColors(true);
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

    m_displayedEvents.clear();

    // TODO: Clear actual event data
}

void RecordingWidget::onExportEvents()
{
    // Create a temporary vector of unique_ptrs for the signal
    // Note: This is not ideal but matches the expected signal signature
    // In practice, the MainWindow should handle the actual export
    std::vector<std::unique_ptr<Core::Event>> emptyEvents;
    emit exportEventsRequested(emptyEvents);
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

void RecordingWidget::addEvent(const Core::Event* event)
{
    if (!event)
        return;

    int row = ui->eventsTableWidget->rowCount();
    ui->eventsTableWidget->insertRow(row);

    // Store the event pointer for potential export
    m_displayedEvents.push_back(event);

    // Format timestamp
    uint64_t timestampMs = event->getTimestampMs();
    QDateTime timestamp = QDateTime::fromMSecsSinceEpoch(timestampMs);
    QString timeStr = timestamp.toString("hh:mm:ss.zzz");

    // Determine event type and details
    QString eventType;
    QString details;

    if (event->isMouseEvent())
    {
        eventType = "Mouse";
        const Core::MouseEventData* mouseData = event->getMouseData();
        if (mouseData)
        {
            QString buttonStr;
            switch (mouseData->button)
            {
            case Core::MouseButton::Left:
                buttonStr = "Left";
                break;
            case Core::MouseButton::Right:
                buttonStr = "Right";
                break;
            case Core::MouseButton::Middle:
                buttonStr = "Middle";
                break;
            case Core::MouseButton::X1:
                buttonStr = "X1";
                break;
            case Core::MouseButton::X2:
                buttonStr = "X2";
                break;
            }

            QString typeStr;
            switch (event->getType())
            {
            case Core::EventType::MouseMove:
                typeStr = "Move";
                break;
            case Core::EventType::MouseClick:
                typeStr = "Click";
                break;
            case Core::EventType::MouseDoubleClick:
                typeStr = "Double Click";
                break;
            case Core::EventType::MouseWheel:
                typeStr = "Wheel";
                break;
            default:
                typeStr = "Unknown";
                break;
            }

            if (event->getType() == Core::EventType::MouseWheel)
            {
                details = QString("%1 - X: %2, Y: %3, Delta: %4")
                            .arg(typeStr)
                            .arg(mouseData->position.x)
                            .arg(mouseData->position.y)
                            .arg(mouseData->wheelDelta);
            }
            else
            {
                details = QString("%1 - Button: %2, X: %3, Y: %4")
                            .arg(typeStr)
                            .arg(buttonStr)
                            .arg(mouseData->position.x)
                            .arg(mouseData->position.y);
            }
        }
        else
        {
            details = "Invalid mouse data";
        }
    }
    else if (event->isKeyboardEvent())
    {
        eventType = "Keyboard";
        const Core::KeyboardEventData* keyData = event->getKeyboardData();
        if (keyData)
        {
            QString typeStr;
            switch (event->getType())
            {
            case Core::EventType::KeyPress:
                typeStr = "Press";
                break;
            case Core::EventType::KeyRelease:
                typeStr = "Release";
                break;
            case Core::EventType::KeyCombination:
                typeStr = "Combination";
                break;
            default:
                typeStr = "Unknown";
                break;
            }

            details = QString("%1 - Key: %2 (%3)")
                        .arg(typeStr)
                        .arg(QString::fromStdString(keyData->keyName))
                        .arg(keyData->keyCode);
        }
        else
        {
            details = "Invalid keyboard data";
        }
    }
    else
    {
        eventType = "Unknown";
        details = "N/A";
    }

    // Add items to table
    ui->eventsTableWidget->setItem(row, 0, new QTableWidgetItem(timeStr));
    ui->eventsTableWidget->setItem(row, 1, new QTableWidgetItem(eventType));
    ui->eventsTableWidget->setItem(row, 2, new QTableWidgetItem(details));

    // Auto-scroll to the latest event
    ui->eventsTableWidget->scrollToBottom();
}

void RecordingWidget::clearEvents()
{
    ui->eventsTableWidget->setRowCount(0);
    m_displayedEvents.clear();
}

void RecordingWidget::setEvents(
  const std::vector<std::unique_ptr<Core::Event>>& events
)
{
    clearEvents();

    for (const auto& event : events)
    {
        if (event)
        {
            addEvent(event.get());
        }
    }
}

} // namespace MouseRecorder::GUI

