#include "PlaybackWidget.hpp"
#include "ui_PlaybackWidget.h"
#include "application/MouseRecorderApp.hpp"
#include "core/IConfiguration.hpp"
#include "storage/EventStorageFactory.hpp"
#include "TestUtils.hpp"
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QTimer>
#include <QMessageBox>
#include <QProgressDialog>
#include <QApplication>
#include <spdlog/spdlog.h>

namespace MouseRecorder::GUI
{

PlaybackWidget::PlaybackWidget(Application::MouseRecorderApp& app,
                               QWidget* parent)
    : QWidget(parent),
      ui(new Ui::PlaybackWidget),
      m_app(app),
      m_loadedEvents(std::make_unique<Core::EventVector>())
{
    ui->setupUi(this);
    setupUI();
}

PlaybackWidget::~PlaybackWidget()
{
    // Stop timer to prevent any pending timer events
    if (m_updateTimer)
    {
        m_updateTimer->stop();
        m_updateTimer->disconnect();
        m_updateTimer->deleteLater();
        m_updateTimer = nullptr;
    }

    // Disconnect all signals to prevent callbacks during destruction
    disconnect();

    // Clear any loaded events to free memory
    m_loadedEvents->clear();

    delete ui;
}

void PlaybackWidget::setupUI()
{
    // Connect signals
    connect(ui->browseFileButton,
            &QPushButton::clicked,
            this,
            &PlaybackWidget::onBrowseFile);
    connect(ui->reloadFileButton,
            &QPushButton::clicked,
            this,
            &PlaybackWidget::onReloadFile);
    connect(
        ui->playButton, &QPushButton::clicked, this, &PlaybackWidget::onPlay);
    connect(
        ui->stopButton, &QPushButton::clicked, this, &PlaybackWidget::onStop);
    connect(ui->speedSlider,
            &QSlider::valueChanged,
            this,
            &PlaybackWidget::onSpeedChanged);
    connect(ui->resetSpeedButton,
            &QPushButton::clicked,
            this,
            &PlaybackWidget::onResetSpeed);
    connect(ui->progressSlider,
            &QSlider::valueChanged,
            this,
            &PlaybackWidget::onProgressChanged);
    connect(ui->loopCheckBox,
            &QCheckBox::toggled,
            this,
            &PlaybackWidget::onLoopToggled);
    connect(ui->loopCountSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &PlaybackWidget::onLoopCountChanged);

    // Initialize table headers
    ui->eventsPreviewTableWidget->setColumnCount(4);
    QStringList headers = {"Index", "Time", "Type", "Details"};
    ui->eventsPreviewTableWidget->setHorizontalHeaderLabels(headers);

    // Setup update timer for playback progress
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer,
            &QTimer::timeout,
            this,
            &PlaybackWidget::updatePlaybackProgress);

    updateUI();
    loadConfigurationSettings();
    updateSpeed();
}

void PlaybackWidget::loadConfigurationSettings()
{
    auto& config = m_app.getConfiguration();

    // Load default playback speed from configuration
    double defaultSpeed =
        config.getDouble(Core::ConfigKeys::DEFAULT_PLAYBACK_SPEED, 1.0);

    // Convert speed to slider value (slider range: 1-50, representing
    // 0.1x-5.0x)
    int sliderValue = static_cast<int>(defaultSpeed * 10.0);

    // Ensure the value is within the valid slider range
    sliderValue = std::clamp(sliderValue, 1, 50);

    // Set the slider value without triggering signals
    ui->speedSlider->blockSignals(true);
    ui->speedSlider->setValue(sliderValue);
    ui->speedSlider->blockSignals(false);

    // Update the event player with the configured speed
    m_app.getEventPlayer().setPlaybackSpeed(defaultSpeed);

    // Load loop playback setting from configuration
    bool loopEnabled = config.getBool(Core::ConfigKeys::LOOP_PLAYBACK, false);

    // Set the loop checkbox without triggering signals
    ui->loopCheckBox->blockSignals(true);
    ui->loopCheckBox->setChecked(loopEnabled);
    ui->loopCheckBox->blockSignals(false);

    spdlog::debug("PlaybackWidget: Loaded default speed from config: {:.1f}x",
                  defaultSpeed);
    spdlog::debug("PlaybackWidget: Loaded loop setting from config: {}",
                  loopEnabled);
}

void PlaybackWidget::onBrowseFile()
{
    QString fileName;

    // In test environment, use a test file to avoid hanging on dialog
    if (TestUtils::isTestEnvironment())
    {
        // Use an existing test file if available, or create a temp one
        QDir tempDir = QDir::temp();
        fileName = tempDir.absoluteFilePath("test_playback.json");

        // Create a simple test file if it doesn't exist
        if (!QFile::exists(fileName))
        {
            QFile testFile(fileName);
            if (testFile.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                testFile.write(
                    "{\"events\":[], \"metadata\":{\"version\":\"1.0\"}}");
                testFile.close();
            }
        }
        spdlog::info("PlaybackWidget: Test environment - using temp file: {}",
                     fileName.toStdString());
    }
    else
    {
        fileName = QFileDialog::getOpenFileName(
            this,
            "Open Recording File",
            "",
            "All Supported (*.json *.mre *.xml);;JSON Files (*.json);;Binary "
            "Files "
            "(*.mre);;XML Files (*.xml)");
    }

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
    if (!m_fileLoaded || m_loadedEvents->empty())
    {
        showWarningMessage("No Events", "Please load a recording file first.");
        return;
    }

    try
    {
        auto& player = m_app.getEventPlayer();

        // Check current state before proceeding
        auto currentState = player.getState();
        if (currentState != Core::PlaybackState::Stopped &&
            currentState != Core::PlaybackState::Completed)
        {
            spdlog::warn("PlaybackWidget: Attempted to play while state is: {}",
                         static_cast<int>(currentState));
            showWarningMessage(
                "Playback Active",
                "Playback is already in progress. Please stop it first.");
            return;
        }

        // Create copies of events instead of moving them
        // This prevents the issue where subsequent plays fail due to empty
        // event vector
        Core::EventVector eventsCopy;
        eventsCopy.reserve(m_loadedEvents->size());

        for (const auto& event : *m_loadedEvents)
        {
            // Create a copy of each event
            auto eventCopy = std::make_unique<Core::Event>(*event);
            eventsCopy.push_back(std::move(eventCopy));
        }

        if (!player.loadEvents(std::move(eventsCopy)))
        {
            showErrorMessage("Playback Error",
                             QString::fromStdString(player.getLastError()));
            return;
        }

        // Set up playback callback
        auto callback =
            [this](Core::PlaybackState state, size_t current, size_t total)
        {
            // This callback runs in the playback thread, so queue the call
            QMetaObject::invokeMethod(
                this,
                [this, state, current, total]()
                {
                    onPlaybackProgress(state, current, total);
                },
                Qt::QueuedConnection);
        };

        if (!player.startPlayback(callback))
        {
            showErrorMessage("Playback Error",
                             QString::fromStdString(player.getLastError()));
            return;
        }

        ui->playButton->setEnabled(false);
        ui->stopButton->setEnabled(true);

        // Start update timer
        m_updateTimer->start(100); // Update every 100ms

        emit playbackStarted();
        spdlog::info("PlaybackWidget: Playback started");
    }
    catch (const std::exception& e)
    {
        showErrorMessage("Error",
                         QString("Failed to start playback: %1").arg(e.what()));
        spdlog::error("PlaybackWidget: Exception during play: {}", e.what());
    }
}

void PlaybackWidget::onStop()
{
    try
    {
        auto& player = m_app.getEventPlayer();
        player.stopPlayback();

        ui->playButton->setEnabled(true);
        ui->stopButton->setEnabled(false);
        ui->progressSlider->setValue(0);

        // Reset time labels to initial state
        if (!m_loadedEvents->empty())
        {
            updateTimeLabels(0, m_loadedEvents->size());
        }

        // Stop update timer
        m_updateTimer->stop();

        emit playbackStopped();
        spdlog::info("PlaybackWidget: Playback stopped");
    }
    catch (const std::exception& e)
    {
        showErrorMessage("Error",
                         QString("Failed to stop playback: %1").arg(e.what()));
        spdlog::error("PlaybackWidget: Exception during stop: {}", e.what());
    }
}

void PlaybackWidget::onSpeedChanged(int value)
{
    updateSpeed();
    try
    {
        auto& player = m_app.getEventPlayer();
        double speed = value / 10.0; // Convert to decimal
        player.setPlaybackSpeed(speed);
        spdlog::debug("PlaybackWidget: Speed changed to {}x", speed);
    }
    catch (const std::exception& e)
    {
        spdlog::error("PlaybackWidget: Exception during speed change: {}",
                      e.what());
    }
}

void PlaybackWidget::onResetSpeed()
{
    auto& config = m_app.getConfiguration();
    double defaultSpeed =
        config.getDouble(Core::ConfigKeys::DEFAULT_PLAYBACK_SPEED, 1.0);

    // Convert speed to slider value and set it
    int sliderValue = static_cast<int>(defaultSpeed * 10.0);
    sliderValue = std::clamp(sliderValue, 1, 50);

    ui->speedSlider->setValue(sliderValue);
    updateSpeed();
}

void PlaybackWidget::onProgressChanged(int value)
{
    try
    {
        auto& player = m_app.getEventPlayer();
        size_t position = static_cast<size_t>(value);
        if (position < player.getTotalEvents())
        {
            player.seekToPosition(position);
            spdlog::debug("PlaybackWidget: Seeked to position {}", position);
        }
    }
    catch (const std::exception& e)
    {
        spdlog::error("PlaybackWidget: Exception during seek: {}", e.what());
    }
}

void PlaybackWidget::onLoopToggled(bool enabled)
{
    try
    {
        auto& player = m_app.getEventPlayer();
        player.setLoopPlayback(enabled);

        // Enable/disable the loop count controls
        ui->loopCountLabel->setEnabled(enabled);
        ui->loopCountSpinBox->setEnabled(enabled);

        // Set the loop count based on the current spin box value
        if (enabled)
        {
            player.setLoopCount(ui->loopCountSpinBox->value());
        }

        spdlog::info("PlaybackWidget: Loop playback set to {}", enabled);
    }
    catch (const std::exception& e)
    {
        spdlog::error("PlaybackWidget: Exception during loop toggle: {}",
                      e.what());
    }
}

void PlaybackWidget::onLoopCountChanged(int count)
{
    try
    {
        auto& player = m_app.getEventPlayer();
        player.setLoopCount(count);
        spdlog::info("PlaybackWidget: Loop count set to {} ({})",
                     count,
                     count == 0 ? "infinite" : "finite");
    }
    catch (const std::exception& e)
    {
        spdlog::error("PlaybackWidget: Exception during loop count change: {}",
                      e.what());
    }
}

void PlaybackWidget::loadFile(const QString& fileName)
{
    spdlog::info("PlaybackWidget: Loading file '{}'", fileName.toStdString());
    m_currentFile = fileName;
    ui->filePathLineEdit->setText(fileName);

    QFileInfo fileInfo(fileName);

    // Show progress dialog for large files (but not in test environment)
    std::unique_ptr<QProgressDialog> progress;
    if (!TestUtils::isTestEnvironment())
    {
        progress = std::make_unique<QProgressDialog>(
            "Loading events...", "Cancel", 0, 0, nullptr);
        progress->setWindowModality(Qt::WindowModal);
        progress->setMinimumDuration(500); // Show after 500ms
        progress->show();
        QApplication::processEvents(); // Allow the progress dialog to show
    }

    try
    {
        // Load events using storage factory
        auto storage = Storage::EventStorageFactory::createStorageFromFilename(
            fileName.toStdString());

        if (!storage)
        {
            if (progress)
            {
                progress->close();
                QApplication::processEvents();
            }
            if (!TestUtils::isTestEnvironment())
            {
                showErrorMessage("Error", "Unsupported file format");
            }
            return;
        }

        Core::EventVector events;
        Core::StorageMetadata metadata;

        if (!storage->loadEvents(fileName.toStdString(), events, metadata))
        {
            if (progress)
            {
                progress->close();
                QApplication::processEvents();
            }
            showErrorMessage("Error",
                             QString::fromStdString(storage->getLastError()));
            return;
        }

        *m_loadedEvents = std::move(events);

        // Update UI with actual data
        ui->fileFormatValue->setText(fileInfo.suffix().toUpper());
        ui->totalEventsValue->setText(QString::number(m_loadedEvents->size()));

        // Calculate duration
        if (!m_loadedEvents->empty())
        {
            uint64_t startTime = m_loadedEvents->front()->getTimestampMs();
            uint64_t endTime = m_loadedEvents->back()->getTimestampMs();
            uint64_t durationMs = endTime - startTime;

            int seconds = static_cast<int>(durationMs / 1000);
            int minutes = seconds / 60;
            int hours = minutes / 60;

            ui->durationValue->setText(
                QString("%1:%2:%3")
                    .arg(hours, 2, 10, QChar('0'))
                    .arg(minutes % 60, 2, 10, QChar('0'))
                    .arg(seconds % 60, 2, 10, QChar('0')));

            // Set progress slider range
            ui->progressSlider->setRange(
                0, static_cast<int>(m_loadedEvents->size() - 1));

            // Initialize time labels
            updateTimeLabels(0, m_loadedEvents->size());
        }
        else
        {
            ui->durationValue->setText("00:00:00");
            ui->progressSlider->setRange(0, 0);
        }

        ui->createdValue->setText(
            fileInfo.birthTime().toString("yyyy-MM-dd hh:mm:ss"));

        // Update events preview table
        ui->eventsPreviewTableWidget->setRowCount(
            std::min(static_cast<int>(m_loadedEvents->size()),
                     100) // Show max 100 events
        );

        for (int i = 0; i < ui->eventsPreviewTableWidget->rowCount(); ++i)
        {
            const auto& event = (*m_loadedEvents)[static_cast<size_t>(i)];
            ui->eventsPreviewTableWidget->setItem(
                i, 0, new QTableWidgetItem(QString::number(i)));

            // Format timestamp
            uint64_t timestamp = event->getTimestampMs();
            uint64_t relativeTime =
                timestamp - m_loadedEvents->front()->getTimestampMs();
            int seconds = static_cast<int>(relativeTime / 1000);
            int milliseconds = static_cast<int>(relativeTime % 1000);

            ui->eventsPreviewTableWidget->setItem(
                i,
                1,
                new QTableWidgetItem(QString("%1.%2s").arg(seconds).arg(
                    milliseconds, 3, 10, QChar('0'))));

            // Event type
            QString typeString;
            switch (event->getType())
            {
            case Core::EventType::MouseMove:
                typeString = "Mouse Move";
                break;
            case Core::EventType::MouseClick:
                typeString = "Mouse Click";
                break;
            case Core::EventType::MouseDoubleClick:
                typeString = "Mouse Double Click";
                break;
            case Core::EventType::MouseWheel:
                typeString = "Mouse Wheel";
                break;
            case Core::EventType::KeyPress:
                typeString = "Key Press";
                break;
            case Core::EventType::KeyRelease:
                typeString = "Key Release";
                break;
            case Core::EventType::KeyCombination:
                typeString = "Key Combination";
                break;
            }

            ui->eventsPreviewTableWidget->setItem(
                i, 2, new QTableWidgetItem(typeString));

            // Event details (simplified)
            QString details = QString::fromStdString(event->toString());
            if (details.length() > 50)
            {
                details = details.left(47) + "...";
            }
            ui->eventsPreviewTableWidget->setItem(
                i, 3, new QTableWidgetItem(details));
        }

        m_fileLoaded = true;
        spdlog::info("PlaybackWidget: Loaded {} events from {}",
                     m_loadedEvents->size(),
                     m_currentFile.toStdString());

        // Close progress dialog before emitting signal
        if (progress)
        {
            progress->close();
            QApplication::processEvents();
        }

        // Emit signal to notify that file was loaded
        emit fileLoaded(m_currentFile);
    }
    catch (const std::exception& e)
    {
        if (progress)
        {
            progress->close();
            QApplication::processEvents();
        }
        showErrorMessage("Error",
                         QString("Failed to load file: %1").arg(e.what()));
        m_fileLoaded = false;
        m_loadedEvents->clear();
    }

    // Re-enable UI
    ui->browseFileButton->setEnabled(true);
    ui->reloadFileButton->setEnabled(m_fileLoaded);
    updateUI();
}

void PlaybackWidget::updateUI()
{
    ui->playButton->setEnabled(m_fileLoaded);
    ui->stopButton->setEnabled(false);
}

void PlaybackWidget::updateSpeed()
{
    double speed = ui->speedSlider->value() / 10.0; // Convert to decimal
    ui->speedValueLabel->setText(QString("%1x").arg(speed, 0, 'f', 1));
}

void PlaybackWidget::updatePlaybackProgress()
{
    try
    {
        auto& player = m_app.getEventPlayer();
        auto state = player.getState();

        if (state == Core::PlaybackState::Playing)
        {
            size_t current = player.getCurrentPosition();
            size_t total = player.getTotalEvents();

            if (total > 0)
            {
                // Update progress slider without triggering signal
                ui->progressSlider->blockSignals(true);
                ui->progressSlider->setValue(static_cast<int>(current));
                ui->progressSlider->blockSignals(false);

                // Update time labels
                updateTimeLabels(current, total);
            }
        }

        // Update UI state if playback finished
        if (state == Core::PlaybackState::Completed ||
            state == Core::PlaybackState::Error)
        {
            ui->playButton->setEnabled(true);
            ui->stopButton->setEnabled(false);
            m_updateTimer->stop();

            if (state == Core::PlaybackState::Completed)
            {
                ui->progressSlider->setValue(ui->progressSlider->maximum());
                // Update time labels to show total duration
                if (!m_loadedEvents->empty())
                {
                    updateTimeLabels(m_loadedEvents->size(),
                                     m_loadedEvents->size());
                }
                emit playbackStopped();
                spdlog::info("PlaybackWidget: Playback completed");

                // No need to reload file - we keep the events in memory for
                // replay
            }
            else if (state == Core::PlaybackState::Error)
            {
                QString errorMsg =
                    QString::fromStdString(player.getLastError());
                showErrorMessage("Playback Error", errorMsg);
                spdlog::error("PlaybackWidget: Playback error: {}",
                              errorMsg.toStdString());
            }
        }
    }
    catch (const std::exception& e)
    {
        spdlog::error("PlaybackWidget: Exception during progress update: {}",
                      e.what());
    }
}

void PlaybackWidget::onPlaybackProgress(Core::PlaybackState state,
                                        size_t current,
                                        size_t total)
{
    // This is called from the playback thread via QueuedConnection
    emit playbackStateChanged(state);

    if (total > 0)
    {
        // Update progress slider
        ui->progressSlider->blockSignals(true);
        ui->progressSlider->setValue(static_cast<int>(current));
        ui->progressSlider->blockSignals(false);
    }
}

void PlaybackWidget::onEventPlayed(const Core::Event& event)
{
    // Optional: highlight current event in the table
    Q_UNUSED(event);
    // Implementation can be added to highlight the current event in the preview
    // table
}

void PlaybackWidget::updateTimeLabels(size_t currentEvent, size_t totalEvents)
{
    if (m_loadedEvents->empty())
    {
        ui->currentTimeLabel->setText("00:00");
        ui->totalTimeLabel->setText("00:00");
        return;
    }

    // Calculate current time based on event timestamps
    auto firstEventTime = (*m_loadedEvents)[0]->getTimestamp();
    auto totalDuration = std::chrono::milliseconds(0);
    auto currentDuration = std::chrono::milliseconds(0);

    if (totalEvents > 0)
    {
        // Calculate total duration (time from first to last event)
        auto lastEventTime = (*m_loadedEvents)[totalEvents - 1]->getTimestamp();
        totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
            lastEventTime - firstEventTime);

        // Calculate current duration based on current event position
        if (currentEvent > 0 && currentEvent <= totalEvents)
        {
            auto currentEventTime =
                (*m_loadedEvents)[currentEvent - 1]->getTimestamp();
            currentDuration =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    currentEventTime - firstEventTime);
        }
    }

    ui->currentTimeLabel->setText(formatTime(currentDuration));
    ui->totalTimeLabel->setText(formatTime(totalDuration));
}

QString PlaybackWidget::formatTime(std::chrono::milliseconds duration)
{
    auto totalSeconds = duration.count() / 1000;
    int minutes = static_cast<int>(totalSeconds / 60);
    int seconds = static_cast<int>(totalSeconds % 60);

    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

void PlaybackWidget::showErrorMessage(const QString& title,
                                      const QString& message)
{
    if (!TestUtils::isTestEnvironment())
    {
        QMessageBox::critical(this, title, message);
    }
}

void PlaybackWidget::showWarningMessage(const QString& title,
                                        const QString& message)
{
    if (!TestUtils::isTestEnvironment())
    {
        QMessageBox::warning(this, title, message);
    }
}

void PlaybackWidget::loadFileRequested(const QString& filename)
{
    // Delegate to the existing loadFile method
    loadFile(filename);
}

} // namespace MouseRecorder::GUI
