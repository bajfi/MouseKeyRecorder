#include "ConfigurationWidget.hpp"
#include "ui_ConfigurationWidget.h"
#include "application/MouseRecorderApp.hpp"
#include "core/IConfiguration.hpp"
#include <QFileDialog>
#include <QKeySequence>
#include <spdlog/spdlog.h>

namespace MouseRecorder::GUI
{

ConfigurationWidget::ConfigurationWidget(
  Application::MouseRecorderApp& app, QWidget* parent
)
  : QWidget(parent), ui(new Ui::ConfigurationWidget), m_app(app)
{
    ui->setupUi(this);
    setupUI();
    loadConfiguration(); // Load current settings from configuration
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
    using namespace MouseRecorder::Core;
    const auto& config = m_app.getConfiguration();

    spdlog::debug("ConfigurationWidget: Loading configuration from backend");

    // Load capture settings
    ui->captureMouseEventsCheckBox->setChecked(
      config.getBool(ConfigKeys::CAPTURE_MOUSE_EVENTS, true)
    );
    ui->captureKeyboardEventsCheckBox->setChecked(
      config.getBool(ConfigKeys::CAPTURE_KEYBOARD_EVENTS, true)
    );
    ui->optimizeMouseMovementsCheckBox->setChecked(
      config.getBool(ConfigKeys::OPTIMIZE_MOUSE_MOVEMENTS, true)
    );
    ui->movementThresholdSpinBox->setValue(
      config.getInt(ConfigKeys::MOUSE_MOVEMENT_THRESHOLD, 5)
    );

    // Load default format (map string to combobox index)
    std::string defaultFormat =
      config.getString(ConfigKeys::DEFAULT_STORAGE_FORMAT, "json");
    if (defaultFormat == "json")
    {
        ui->defaultFormatComboBox->setCurrentIndex(0);
    }
    else if (defaultFormat == "xml")
    {
        ui->defaultFormatComboBox->setCurrentIndex(1);
    }
    else if (defaultFormat == "binary")
    {
        ui->defaultFormatComboBox->setCurrentIndex(2);
    }

    // Load playback settings
    ui->defaultSpeedSpinBox->setValue(
      config.getDouble(ConfigKeys::DEFAULT_PLAYBACK_SPEED, 1.0)
    );
    ui->loopPlaybackCheckBox->setChecked(
      config.getBool(ConfigKeys::LOOP_PLAYBACK, false)
    );
    ui->showPlaybackCursorCheckBox->setChecked(
      config.getBool(ConfigKeys::SHOW_PLAYBACK_CURSOR, true)
    );

    // Load keyboard shortcuts
    ui->startRecordingShortcutEdit->setKeySequence(QKeySequence(
      QString::fromStdString(
        config.getString(ConfigKeys::SHORTCUT_START_RECORDING, "Ctrl+R")
      )
    ));
    ui->stopRecordingShortcutEdit->setKeySequence(QKeySequence(
      QString::fromStdString(
        config.getString(ConfigKeys::SHORTCUT_STOP_RECORDING, "Ctrl+Shift+R")
      )
    ));
    ui->startPlaybackShortcutEdit->setKeySequence(QKeySequence(
      QString::fromStdString(
        config.getString(ConfigKeys::SHORTCUT_START_PLAYBACK, "Ctrl+P")
      )
    ));
    ui->stopPlaybackShortcutEdit->setKeySequence(QKeySequence(
      QString::fromStdString(
        config.getString(ConfigKeys::SHORTCUT_STOP_PLAYBACK, "Ctrl+Shift+P")
      )
    ));
    ui->pausePlaybackShortcutEdit->setKeySequence(QKeySequence(
      QString::fromStdString(
        config.getString(ConfigKeys::SHORTCUT_PAUSE_PLAYBACK, "Ctrl+Space")
      )
    ));

    // Load logging settings
    std::string logLevel = config.getString(ConfigKeys::LOG_LEVEL, "info");
    int logLevelIndex = 4; // Default to "info"
    if (logLevel == "trace")
        logLevelIndex = 0;
    else if (logLevel == "debug")
        logLevelIndex = 1;
    else if (logLevel == "info")
        logLevelIndex = 2;
    else if (logLevel == "warn")
        logLevelIndex = 3;
    else if (logLevel == "error")
        logLevelIndex = 4;
    else if (logLevel == "critical")
        logLevelIndex = 5;
    ui->logLevelComboBox->setCurrentIndex(logLevelIndex);

    ui->logToFileCheckBox->setChecked(
      config.getBool(ConfigKeys::LOG_TO_FILE, false)
    );
    ui->logFilePathLineEdit->setText(
      QString::fromStdString(
        config.getString(ConfigKeys::LOG_FILE_PATH, "mouserecorder.log")
      )
    );

    updateUI();
    spdlog::debug("ConfigurationWidget: Configuration loaded successfully");
}

void ConfigurationWidget::saveConfiguration()
{
    using namespace MouseRecorder::Core;
    auto& config = m_app.getConfiguration();

    spdlog::info("ConfigurationWidget: Saving configuration to backend");

    // Save capture settings
    config.setBool(
      ConfigKeys::CAPTURE_MOUSE_EVENTS,
      ui->captureMouseEventsCheckBox->isChecked()
    );
    config.setBool(
      ConfigKeys::CAPTURE_KEYBOARD_EVENTS,
      ui->captureKeyboardEventsCheckBox->isChecked()
    );
    config.setBool(
      ConfigKeys::OPTIMIZE_MOUSE_MOVEMENTS,
      ui->optimizeMouseMovementsCheckBox->isChecked()
    );
    config.setInt(
      ConfigKeys::MOUSE_MOVEMENT_THRESHOLD,
      ui->movementThresholdSpinBox->value()
    );

    // Save default format (map combobox index to string)
    std::string formatString = "json";
    int formatIndex = ui->defaultFormatComboBox->currentIndex();
    if (formatIndex == 0)
        formatString = "json";
    else if (formatIndex == 1)
        formatString = "xml";
    else if (formatIndex == 2)
        formatString = "binary";
    config.setString(ConfigKeys::DEFAULT_STORAGE_FORMAT, formatString);

    // Save playback settings
    config.setDouble(
      ConfigKeys::DEFAULT_PLAYBACK_SPEED, ui->defaultSpeedSpinBox->value()
    );
    config.setBool(
      ConfigKeys::LOOP_PLAYBACK, ui->loopPlaybackCheckBox->isChecked()
    );
    config.setBool(
      ConfigKeys::SHOW_PLAYBACK_CURSOR,
      ui->showPlaybackCursorCheckBox->isChecked()
    );

    // Save keyboard shortcuts
    config.setString(
      ConfigKeys::SHORTCUT_START_RECORDING,
      ui->startRecordingShortcutEdit->keySequence().toString().toStdString()
    );
    config.setString(
      ConfigKeys::SHORTCUT_STOP_RECORDING,
      ui->stopRecordingShortcutEdit->keySequence().toString().toStdString()
    );
    config.setString(
      ConfigKeys::SHORTCUT_START_PLAYBACK,
      ui->startPlaybackShortcutEdit->keySequence().toString().toStdString()
    );
    config.setString(
      ConfigKeys::SHORTCUT_STOP_PLAYBACK,
      ui->stopPlaybackShortcutEdit->keySequence().toString().toStdString()
    );
    config.setString(
      ConfigKeys::SHORTCUT_PAUSE_PLAYBACK,
      ui->pausePlaybackShortcutEdit->keySequence().toString().toStdString()
    );

    // Save logging settings
    const QStringList logLevels = {
      "trace", "debug", "info", "warn", "error", "critical"
    };
    int logLevelIndex = ui->logLevelComboBox->currentIndex();
    if (logLevelIndex >= 0 && logLevelIndex < logLevels.size())
    {
        config.setString(
          ConfigKeys::LOG_LEVEL, logLevels[logLevelIndex].toStdString()
        );
    }

    config.setBool(ConfigKeys::LOG_TO_FILE, ui->logToFileCheckBox->isChecked());
    config.setString(
      ConfigKeys::LOG_FILE_PATH, ui->logFilePathLineEdit->text().toStdString()
    );

    // Force save to file immediately
    bool success = config.saveToFile("");
    if (success)
    {
        spdlog::info("ConfigurationWidget: Configuration saved successfully");
    }
    else
    {
        spdlog::error(
          "ConfigurationWidget: Failed to save configuration: {}",
          config.getLastError()
        );
    }
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

