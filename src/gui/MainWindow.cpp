#include "MainWindow.hpp"
#include "ui_MainWindow.h"
#include "RecordingWidget.hpp"
#include "PlaybackWidget.hpp"
#include "ConfigurationWidget.hpp"
#include "../core/QtConfiguration.hpp"
#include "../core/IEventStorage.hpp"
#include "../storage/EventStorageFactory.hpp"
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QStatusBar>
#include <QMetaObject>
#include <QTimer>
#include <QFileInfo>
#include <spdlog/spdlog.h>
#include <filesystem>

namespace MouseRecorder::GUI
{

MainWindow::MainWindow(Application::MouseRecorderApp& app, QWidget* parent)
  : QMainWindow(parent), ui(new Ui::MainWindow), m_app(app)
{
    ui->setupUi(this);

    // Ensure the application quits when the main window closes
    setAttribute(Qt::WA_QuitOnClose, true);
    QApplication::setQuitOnLastWindowClosed(true);

    setupWidgets();
    setupActions();
    setupStatusBar();
    updateUI();
    updateWindowTitle();

    spdlog::info("MainWindow: Initialized");
}

MainWindow::~MainWindow()
{
    delete ui;
    spdlog::info("MainWindow: Destroyed");
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    spdlog::info("MainWindow: closeEvent triggered");

    if (m_modified)
    {
        QMessageBox::StandardButton reply = QMessageBox::question(
          this,
          "Unsaved Changes",
          "You have unsaved changes. Do you want to save before closing?",
          QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );

        if (reply == QMessageBox::Save)
        {
            onSaveFile();
        }
        else if (reply == QMessageBox::Cancel)
        {
            event->ignore();
            return;
        }
        // If Discard, continue with closing
    }

    // Stop any active operations
    try
    {
        if (m_app.getEventRecorder().isRecording())
        {
            spdlog::info("MainWindow: Stopping active recording before close");
            m_app.getEventRecorder().stopRecording();
        }

        if (m_app.getEventPlayer().getState() != Core::PlaybackState::Stopped)
        {
            spdlog::info("MainWindow: Stopping active playback before close");
            m_app.getEventPlayer().stopPlayback();
        }
    }
    catch (const std::exception& e)
    {
        spdlog::warn(
          "MainWindow: Error stopping operations during close: {}", e.what()
        );
    }

    // Note: Window geometry saving in closeEvent was causing hangs due to
    // configuration callbacks during shutdown. The geometry will be saved
    // periodically by the main application or on normal configuration saves.
    spdlog::info(
      "MainWindow: Skipping window geometry save during close to avoid "
      "shutdown hang"
    );

    spdlog::info("MainWindow: About to call application shutdown");

    // Call application shutdown to clean up properly
    m_app.shutdown();

    spdlog::info("MainWindow: Application shutdown completed");
    event->accept();
}

void MainWindow::setupWidgets()
{
    // Create and setup custom widgets
    m_recordingWidget = new RecordingWidget(this);
    m_playbackWidget = new PlaybackWidget(this);
    m_configurationWidget = new ConfigurationWidget(this);

    // Replace the placeholder widgets in the tabs
    ui->recordingLayout->addWidget(m_recordingWidget);
    ui->playbackLayout->addWidget(m_playbackWidget);
    ui->configurationLayout->addWidget(m_configurationWidget);

    // Connect recording widget signals to actual recording functionality
    connect(
      m_recordingWidget,
      &RecordingWidget::recordingStarted,
      this,
      &MainWindow::onStartRecording
    );
    connect(
      m_recordingWidget,
      &RecordingWidget::recordingStopped,
      this,
      &MainWindow::onStopRecording
    );
    connect(
      m_recordingWidget,
      &RecordingWidget::exportEventsRequested,
      this,
      &MainWindow::onExportEvents
    );

    // Connect playback widget signals
    connect(
      m_playbackWidget,
      &PlaybackWidget::playbackStarted,
      this,
      &MainWindow::onPlaybackStarted
    );
    connect(
      m_playbackWidget,
      &PlaybackWidget::playbackStopped,
      this,
      &MainWindow::onPlaybackStopped
    );
}

void MainWindow::setupActions()
{
    // File menu
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::onNewFile);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::onOpenFile);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::onSaveFile);
    connect(
      ui->actionSaveAs, &QAction::triggered, this, &MainWindow::onSaveAsFile
    );
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::onExit);

    // Help menu
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::onAbout);
    connect(
      ui->actionAboutQt, &QAction::triggered, this, &MainWindow::onAboutQt
    );

    // Recording actions
    connect(
      ui->actionStartRecording,
      &QAction::triggered,
      this,
      &MainWindow::onStartRecording
    );
    connect(
      ui->actionStopRecording,
      &QAction::triggered,
      this,
      &MainWindow::onStopRecording
    );

    // Playback actions
    connect(
      ui->actionStartPlayback,
      &QAction::triggered,
      this,
      &MainWindow::onStartPlayback
    );
    connect(
      ui->actionPausePlayback,
      &QAction::triggered,
      this,
      &MainWindow::onPausePlayback
    );
    connect(
      ui->actionStopPlayback,
      &QAction::triggered,
      this,
      &MainWindow::onStopPlayback
    );
}

void MainWindow::setupStatusBar()
{
    ui->statusLabel->setText("Ready");
    ui->connectionLabel->setText("Connected");
}

void MainWindow::updateUI()
{
    bool isRecording = m_app.getEventRecorder().isRecording();
    bool isPlayingBack =
      (m_app.getEventPlayer().getState() == Core::PlaybackState::Playing);

    // Update recording actions
    ui->actionStartRecording->setEnabled(!isRecording && !isPlayingBack);
    ui->actionStopRecording->setEnabled(isRecording);

    // Update playback actions
    ui->actionStartPlayback->setEnabled(
      !isRecording && !isPlayingBack && !m_currentFile.isEmpty()
    );
    ui->actionPausePlayback->setEnabled(isPlayingBack);
    ui->actionStopPlayback->setEnabled(isPlayingBack);

    // Update status
    if (isRecording)
    {
        ui->statusLabel->setText("Recording...");
    }
    else if (isPlayingBack)
    {
        ui->statusLabel->setText("Playing back...");
    }
    else
    {
        ui->statusLabel->setText("Ready");
    }
}

void MainWindow::updateWindowTitle()
{
    QString title = Application::MouseRecorderApp::getApplicationName().c_str();

    if (!m_currentFile.isEmpty())
    {
        QFileInfo fileInfo(m_currentFile);
        title += " - " + fileInfo.fileName();

        if (m_modified)
        {
            title += " *";
        }
    }

    setWindowTitle(title);
}

void MainWindow::onNewFile()
{
    // TODO: Clear current recording/playback
    m_currentFile.clear();
    m_modified = false;
    updateWindowTitle();
    updateUI();

    ui->statusLabel->setText("New session started");
}

void MainWindow::onOpenFile()
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
        // TODO: Load the file
        m_currentFile = fileName;
        m_modified = false;
        updateWindowTitle();
        updateUI();

        ui->statusLabel->setText(
          "File loaded: " + QFileInfo(fileName).fileName()
        );
    }
}

void MainWindow::onSaveFile()
{
    if (m_currentFile.isEmpty())
    {
        onSaveAsFile();
    }
    else
    {
        // TODO: Save to current file
        m_modified = false;
        updateWindowTitle();
        ui->statusLabel->setText("File saved");
    }
}

void MainWindow::onSaveAsFile()
{
    QString fileName = QFileDialog::getSaveFileName(
      this,
      "Save Recording File",
      "",
      "JSON Files (*.json);;Binary Files (*.mre);;XML Files (*.xml)"
    );

    if (!fileName.isEmpty())
    {
        // TODO: Save to the specified file
        m_currentFile = fileName;
        m_modified = false;
        updateWindowTitle();
        ui->statusLabel->setText(
          "File saved: " + QFileInfo(fileName).fileName()
        );
    }
}

void MainWindow::onExit()
{
    close();
}

void MainWindow::onAbout()
{
    QString aboutText =
      QString(
        "<h3>%1 v%2</h3>"
        "<p>Cross-platform Mouse and Keyboard Event Recorder</p>"
        "<p>Built with modern C++23 and Qt5</p>"
        "<p>Copyright © 2024 MouseRecorder Team</p>"
      )
        .arg(Application::MouseRecorderApp::getApplicationName().c_str())
        .arg(Application::MouseRecorderApp::getVersion().c_str());

    QMessageBox::about(this, "About MouseRecorder", aboutText);
}

void MainWindow::onAboutQt()
{
    QMessageBox::aboutQt(this);
}

void MainWindow::onStartRecording()
{
    // Create event callback that stores events
    auto eventCallback = [this](std::unique_ptr<Core::Event> event)
    {
        {
            std::lock_guard<std::mutex> lock(m_eventsMutex);
            // Add event to the UI display (before moving to storage)
            if (m_recordingWidget && event)
            {
                QMetaObject::invokeMethod(
                  m_recordingWidget,
                  [this, rawEvent = event.get()]()
                  {
                      m_recordingWidget->addEvent(rawEvent);
                  },
                  Qt::QueuedConnection
                );
            }

            m_recordedEvents.push_back(std::move(event));
        }

        // Update UI on the main thread
        QMetaObject::invokeMethod(
          this,
          [this]()
          {
              updateRecordingStatistics();
          },
          Qt::QueuedConnection
        );
    };

    if (m_app.getEventRecorder().startRecording(eventCallback))
    {
        m_recordedEvents.clear();
        // Clear the recording widget display
        if (m_recordingWidget)
        {
            m_recordingWidget->clearEvents();
        }
        ui->statusLabel->setText("Recording started");
        updateUI();

        // Call the handler that updates all UI components
        onRecordingStarted();
    }
    else
    {
        QMessageBox::critical(
          this,
          "Recording Error",
          QString("Failed to start recording: %1")
            .arg(
              QString::fromStdString(m_app.getEventRecorder().getLastError())
            )
        );
    }
}

void MainWindow::onStopRecording()
{
    try
    {
        if (m_app.getEventRecorder().isRecording())
        {
            m_app.getEventRecorder().stopRecording();
            m_modified = true;
            updateWindowTitle();
        }
        ui->statusLabel->setText("Recording stopped");
        updateUI();

        // Call the handler that updates all UI components
        onRecordingStopped();
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(
          this,
          "Recording Error",
          QString("Failed to stop recording: %1").arg(e.what())
        );
    }
}

void MainWindow::onExportEvents()
{
    if (m_recordedEvents.empty())
    {
        QMessageBox::information(
          this,
          "Export Events",
          "No events to export. Please record some events first."
        );
        return;
    }

    QString selectedFilter;
    QString fileName = QFileDialog::getSaveFileName(
      this,
      "Export Events",
      QString(),
      QString::fromStdString(
        Storage::EventStorageFactory::getFileDialogFilter()
      ),
      &selectedFilter
    );

    if (fileName.isEmpty())
        return;

    try
    {
        // Determine format from selected filter or file extension
        std::unique_ptr<Core::IEventStorage> storage;

        // First try to determine format from selected filter
        if (selectedFilter.contains("JSON"))
        {
            storage = Storage::EventStorageFactory::createStorage(
              Core::StorageFormat::Json
            );
        }
        else if (selectedFilter.contains("XML"))
        {
            storage = Storage::EventStorageFactory::createStorage(
              Core::StorageFormat::Xml
            );
        }
        else if (selectedFilter.contains("Binary"))
        {
            storage = Storage::EventStorageFactory::createStorage(
              Core::StorageFormat::Binary
            );
        }
        else
        {
            // Fallback to file extension-based detection
            storage = Storage::EventStorageFactory::createStorageFromFilename(
              fileName.toStdString()
            );
        }
        if (!storage)
        {
            QMessageBox::critical(
              this,
              "Export Error",
              "Unsupported file format. Please use .json, .xml, or .mre "
              "extension."
            );
            return;
        }

        // Copy events for export (since saveEvents expects const reference)
        std::vector<std::unique_ptr<Core::Event>> eventsToExport;
        {
            std::lock_guard<std::mutex> lock(m_eventsMutex);
            for (const auto& event : m_recordedEvents)
            {
                if (event)
                {
                    // Create a copy of the event for export
                    // This is a simplification - in practice you might want to
                    // implement a proper clone method for Event
                    eventsToExport.emplace_back(
                      std::make_unique<Core::Event>(*event)
                    );
                }
            }
        }

        std::string file_with_suffix(fileName.toStdString());
        if (std::filesystem::path(fileName.toStdString()).extension() !=
            storage->getFileExtension())
        {
            file_with_suffix += storage->getFileExtension();
        }
        if (storage->saveEvents(eventsToExport, file_with_suffix))
        {
            QMessageBox::information(
              this,
              "Export Complete",
              QString("Successfully exported %1 events to %2")
                .arg(eventsToExport.size())
                .arg(QFileInfo(file_with_suffix.c_str()).fileName())
            );
        }
        else
        {
            QMessageBox::critical(
              this,
              "Export Error",
              QString("Failed to export events: %1")
                .arg(QString::fromStdString(storage->getLastError()))
            );
        }
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(
          this,
          "Export Error",
          QString("Failed to export events: %1").arg(e.what())
        );
    }
}

void MainWindow::onStartPlayback()
{
    try
    {
        if (m_recordedEvents.empty() && m_currentFile.isEmpty())
        {
            QMessageBox::information(
              this,
              "No Events",
              "No events available for playback. Please record some events or "
              "load a file first."
            );
            return;
        }

        // Create playback callback
        auto playbackCallback =
          [this](
            Core::PlaybackState state, size_t currentIndex, size_t totalEvents
          )
        {
            QMetaObject::invokeMethod(
              this,
              [this, state, currentIndex, totalEvents]()
              {
                  if (m_playbackWidget)
                  {
                      // Update playback progress in the widget
                      // For now just update the status
                      QString status = QString("Playing: %1/%2")
                                         .arg(currentIndex)
                                         .arg(totalEvents);
                      ui->statusLabel->setText(status);
                  }

                  if (state == Core::PlaybackState::Completed ||
                      state == Core::PlaybackState::Error)
                  {
                      updateUI();
                  }
              }
            );
        };

        // Load events into player
        std::vector<std::unique_ptr<Core::Event>> eventsToPlay;
        if (!m_recordedEvents.empty())
        {
            // Copy recorded events for playback
            for (const auto& event : m_recordedEvents)
            {
                // Create a copy of the event
                // This is a simplified approach - in a real implementation
                // you'd want a proper copy method
                auto eventCopy = std::make_unique<Core::Event>(*event);
                eventsToPlay.push_back(std::move(eventCopy));
            }
        }
        else if (!m_currentFile.isEmpty())
        {
            // Load events from file (placeholder - would need actual file
            // loading)
            QMessageBox::information(
              this,
              "File Playback",
              "File playback will be implemented when storage functionality is "
              "complete."
            );
            return;
        }

        if (m_app.getEventPlayer().loadEvents(std::move(eventsToPlay)))
        {
            if (m_app.getEventPlayer().startPlayback(playbackCallback))
            {
                ui->statusLabel->setText("Playback started");
                updateUI();
            }
            else
            {
                QMessageBox::critical(
                  this,
                  "Playback Error",
                  QString("Failed to start playback: %1")
                    .arg(
                      QString::fromStdString(
                        m_app.getEventPlayer().getLastError()
                      )
                    )
                );
            }
        }
        else
        {
            QMessageBox::critical(
              this,
              "Playback Error",
              QString("Failed to load events for playback: %1")
                .arg(
                  QString::fromStdString(m_app.getEventPlayer().getLastError())
                )
            );
        }
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(
          this,
          "Playback Error",
          QString("Failed to start playback: %1").arg(e.what())
        );
    }
}

void MainWindow::onPausePlayback()
{
    try
    {
        m_app.getEventPlayer().pausePlayback();
        ui->statusLabel->setText("Playback paused");
        updateUI();
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(
          this,
          "Playback Error",
          QString("Failed to pause playback: %1").arg(e.what())
        );
    }
}

void MainWindow::onStopPlayback()
{
    try
    {
        m_app.getEventPlayer().stopPlayback();
        ui->statusLabel->setText("Playback stopped");
        updateUI();
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(
          this,
          "Playback Error",
          QString("Failed to stop playback: %1").arg(e.what())
        );
    }
}

void MainWindow::onRecordingStarted()
{
    // This method is now called when recording is actually started by
    // onStartRecording() Update the recording widget to reflect the actual
    // recording state
    spdlog::info("MainWindow: Recording started - updating UI");

    m_modified = true;
    updateWindowTitle();
    updateUI();
    ui->statusLabel->setText("Recording in progress");

    // Update the recording widget's UI to show recording state
    if (m_recordingWidget)
    {
        // The widget should already be in the right state since it triggered
        // this
        spdlog::debug("MainWindow: Recording widget updated");
    }
}

void MainWindow::onRecordingStopped()
{
    // This method is now called when recording is actually stopped by
    // onStopRecording()
    spdlog::info("MainWindow: Recording stopped - updating UI");

    m_modified = true;
    updateWindowTitle();
    updateUI();
    ui->statusLabel->setText("Recording stopped");

    // Update the recording widget's UI to show stopped state
    if (m_recordingWidget)
    {
        spdlog::debug("MainWindow: Recording widget updated");
    }
}

void MainWindow::onPlaybackStarted()
{
    updateUI();
    ui->statusLabel->setText("Playback started from widget");
}

void MainWindow::onPlaybackStopped()
{
    updateUI();
    ui->statusLabel->setText("Playback stopped from widget");
}

void MainWindow::updateRecordingStatistics()
{
    if (!m_recordingWidget)
    {
        spdlog::warn(
          "MainWindow: updateRecordingStatistics called but m_recordingWidget "
          "is null"
        );
        return;
    }

    std::lock_guard<std::mutex> lock(m_eventsMutex);

    size_t totalEvents = m_recordedEvents.size();
    size_t mouseEvents = 0;
    size_t keyboardEvents = 0;

    for (const auto& event : m_recordedEvents)
    {
        if (event && event->isMouseEvent())
        {
            mouseEvents++;
        }
        else if (event && event->isKeyboardEvent())
        {
            keyboardEvents++;
        }
    }

    spdlog::debug(
      "MainWindow: Updating statistics - Total: {}, Mouse: {}, Keyboard: {}",
      totalEvents,
      mouseEvents,
      keyboardEvents
    );

    m_recordingWidget->updateStatistics(
      totalEvents, mouseEvents, keyboardEvents
    );
}

} // namespace MouseRecorder::GUI

