#include "PlaybackWidget.hpp"
#include "ui_PlaybackWidget.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QDateTime>

namespace MouseRecorder::GUI
{

PlaybackWidget::PlaybackWidget(QWidget* parent)
  : QWidget(parent), ui(new Ui::PlaybackWidget)
{
    ui->setupUi(this);
    setupUI();
}

PlaybackWidget::~PlaybackWidget()
{
    delete ui;
}

void PlaybackWidget::setupUI()
{
    // Connect signals
    connect(
      ui->browseFileButton,
      &QPushButton::clicked,
      this,
      &PlaybackWidget::onBrowseFile
    );
    connect(
      ui->reloadFileButton,
      &QPushButton::clicked,
      this,
      &PlaybackWidget::onReloadFile
    );
    connect(
      ui->playButton, &QPushButton::clicked, this, &PlaybackWidget::onPlay
    );
    connect(
      ui->pauseButton, &QPushButton::clicked, this, &PlaybackWidget::onPause
    );
    connect(
      ui->stopButton, &QPushButton::clicked, this, &PlaybackWidget::onStop
    );
    connect(
      ui->speedSlider,
      &QSlider::valueChanged,
      this,
      &PlaybackWidget::onSpeedChanged
    );
    connect(
      ui->resetSpeedButton,
      &QPushButton::clicked,
      this,
      &PlaybackWidget::onResetSpeed
    );
    connect(
      ui->progressSlider,
      &QSlider::valueChanged,
      this,
      &PlaybackWidget::onProgressChanged
    );

    // Initialize table headers
    ui->eventsPreviewTableWidget->setColumnCount(4);
    QStringList headers = {"Index", "Time", "Type", "Details"};
    ui->eventsPreviewTableWidget->setHorizontalHeaderLabels(headers);

    updateUI();
    updateSpeed();
}

void PlaybackWidget::onBrowseFile()
{
    QString fileName = QFileDialog::getOpenFileName(
      this,
      "Open Recording File",
      "",
      "All Supported (*.json *.mre *.xml);;JSON Files (*.json);;Binary Files "
      "(*.mre);;XML Files (*.xml)"
    );

    if (!fileName.isEmpty())
    {
        loadFile(fileName);
    }
}

void PlaybackWidget::onReloadFile()
{
    if (!m_currentFile.isEmpty())
    {
        loadFile(m_currentFile);
    }
}

void PlaybackWidget::onPlay()
{
    // TODO: Start playback
    ui->playButton->setEnabled(false);
    ui->pauseButton->setEnabled(true);
    ui->stopButton->setEnabled(true);

    emit playbackStarted();
}

void PlaybackWidget::onPause()
{
    // TODO: Pause playback
    ui->playButton->setEnabled(true);
    ui->pauseButton->setEnabled(false);
}

void PlaybackWidget::onStop()
{
    // TODO: Stop playback
    ui->playButton->setEnabled(true);
    ui->pauseButton->setEnabled(false);
    ui->stopButton->setEnabled(false);
    ui->progressSlider->setValue(0);

    emit playbackStopped();
}

void PlaybackWidget::onSpeedChanged(int /*value*/)
{
    updateSpeed();
    // TODO: Update actual playback speed
}

void PlaybackWidget::onResetSpeed()
{
    ui->speedSlider->setValue(10); // 1.0x speed
    updateSpeed();
}

void PlaybackWidget::onProgressChanged(int value)
{
    // TODO: Seek to position
    Q_UNUSED(value)
}

void PlaybackWidget::loadFile(const QString& fileName)
{
    m_currentFile = fileName;
    ui->filePathLineEdit->setText(fileName);

    QFileInfo fileInfo(fileName);

    // TODO: Actually load and parse the file
    // For now, just update the UI with placeholder data
    ui->fileFormatValue->setText(fileInfo.suffix().toUpper());
    ui->totalEventsValue->setText("0");
    ui->durationValue->setText("00:00:00");
    ui->createdValue->setText(
      fileInfo.birthTime().toString("yyyy-MM-dd hh:mm:ss")
    );

    m_fileLoaded = true;
    ui->reloadFileButton->setEnabled(true);

    updateUI();
}

void PlaybackWidget::updateUI()
{
    ui->playButton->setEnabled(m_fileLoaded);
    ui->pauseButton->setEnabled(false);
    ui->stopButton->setEnabled(false);
}

void PlaybackWidget::updateSpeed()
{
    double speed = ui->speedSlider->value() / 10.0; // Convert to decimal
    ui->speedValueLabel->setText(QString("%1x").arg(speed, 0, 'f', 1));
}

} // namespace MouseRecorder::GUI

