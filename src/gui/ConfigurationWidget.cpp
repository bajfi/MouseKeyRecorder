#include "ConfigurationWidget.hpp"
#include "ui_ConfigurationWidget.h"
#include <QFileDialog>
#include <QKeySequence>

namespace MouseRecorder::GUI
{

ConfigurationWidget::ConfigurationWidget(QWidget* parent)
  : QWidget(parent), ui(new Ui::ConfigurationWidget)
{
    ui->setupUi(this);
    setupUI();
}

ConfigurationWidget::~ConfigurationWidget()
{
    delete ui;
}

void ConfigurationWidget::setupUI()
{
    // Connect signals
    connect(
      ui->restoreDefaultsButton,
      &QPushButton::clicked,
      this,
      &ConfigurationWidget::onRestoreDefaults
    );
    connect(
      ui->applyButton,
      &QPushButton::clicked,
      this,
      &ConfigurationWidget::onApply
    );
    connect(
      ui->browseLogFileButton,
      &QPushButton::clicked,
      this,
      &ConfigurationWidget::onBrowseLogFile
    );

    // Setup default values
    ui->captureMouseEventsCheckBox->setChecked(true);
    ui->captureKeyboardEventsCheckBox->setChecked(true);
    ui->optimizeMouseMovementsCheckBox->setChecked(true);
    ui->movementThresholdSpinBox->setValue(5);
    ui->defaultFormatComboBox->setCurrentIndex(0); // JSON

    ui->defaultSpeedSpinBox->setValue(1.0);
    ui->loopPlaybackCheckBox->setChecked(false);
    ui->showPlaybackCursorCheckBox->setChecked(true);

    // Setup default shortcuts
    ui->startRecordingShortcutEdit->setKeySequence(QKeySequence("Ctrl+R"));
    ui->stopRecordingShortcutEdit->setKeySequence(QKeySequence("Ctrl+Shift+R"));
    ui->startPlaybackShortcutEdit->setKeySequence(QKeySequence("Ctrl+P"));
    ui->stopPlaybackShortcutEdit->setKeySequence(QKeySequence("Ctrl+Shift+P"));
    ui->pausePlaybackShortcutEdit->setKeySequence(QKeySequence("Ctrl+Space"));

    ui->logLevelComboBox->setCurrentIndex(4); // Info
    ui->logToFileCheckBox->setChecked(false);
    ui->logFilePathLineEdit->setText("mouserecorder.log");

    updateUI();
}

void ConfigurationWidget::loadConfiguration()
{
    // TODO: Load configuration from actual configuration manager
    updateUI();
}

void ConfigurationWidget::saveConfiguration()
{
    // TODO: Save configuration to actual configuration manager
}

void ConfigurationWidget::onRestoreDefaults()
{
    ui->captureMouseEventsCheckBox->setChecked(true);
    ui->captureKeyboardEventsCheckBox->setChecked(true);
    ui->optimizeMouseMovementsCheckBox->setChecked(true);
    ui->movementThresholdSpinBox->setValue(5);
    ui->defaultFormatComboBox->setCurrentIndex(0);

    ui->defaultSpeedSpinBox->setValue(1.0);
    ui->loopPlaybackCheckBox->setChecked(false);
    ui->showPlaybackCursorCheckBox->setChecked(true);

    ui->startRecordingShortcutEdit->setKeySequence(QKeySequence("Ctrl+R"));
    ui->stopRecordingShortcutEdit->setKeySequence(QKeySequence("Ctrl+Shift+R"));
    ui->startPlaybackShortcutEdit->setKeySequence(QKeySequence("Ctrl+P"));
    ui->stopPlaybackShortcutEdit->setKeySequence(QKeySequence("Ctrl+Shift+P"));
    ui->pausePlaybackShortcutEdit->setKeySequence(QKeySequence("Ctrl+Space"));

    ui->logLevelComboBox->setCurrentIndex(4);
    ui->logToFileCheckBox->setChecked(false);
    ui->logFilePathLineEdit->setText("mouserecorder.log");

    updateUI();
}

void ConfigurationWidget::onApply()
{
    saveConfiguration();
}

void ConfigurationWidget::onBrowseLogFile()
{
    QString fileName = QFileDialog::getSaveFileName(
      this,
      "Select Log File",
      ui->logFilePathLineEdit->text(),
      "Log Files (*.log);;Text Files (*.txt);;All Files (*)"
    );

    if (!fileName.isEmpty())
    {
        ui->logFilePathLineEdit->setText(fileName);
    }
}

void ConfigurationWidget::updateUI()
{
    // Enable/disable controls based on settings
    ui->movementThresholdSpinBox->setEnabled(
      ui->optimizeMouseMovementsCheckBox->isChecked()
    );
    ui->logFilePathLineEdit->setEnabled(ui->logToFileCheckBox->isChecked());
    ui->browseLogFileButton->setEnabled(ui->logToFileCheckBox->isChecked());
}

} // namespace MouseRecorder::GUI

