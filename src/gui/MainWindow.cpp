#include "MainWindow.hpp"
#include "ui_MainWindow.h"
#include "RecordingWidget.hpp"
#include "PlaybackWidget.hpp"
#include "ConfigurationWidget.hpp"
#include "../core/QtConfiguration.hpp"
#include "../core/IEventStorage.hpp"
#include "../storage/EventStorageFactory.hpp"
#include "TestUtils.hpp"
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QCloseEvent>
#include <QStatusBar>
#include <QMetaObject>
#include <QTimer>
#include <QFileInfo>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QEvent>
#include <QShortcut>
#include <QKeySequence>
#include <QSettings>
#include <QDialog>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <spdlog/spdlog.h>
#include <filesystem>

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#endif

namespace MouseRecorder::GUI
{

MainWindow::MainWindow(Application::MouseRecorderApp& app, QWidget* parent)
  : QMainWindow(parent), ui(new Ui::MainWindow), m_app(app)
{
    ui->setupUi(this);
    ui->actionAbout->setIcon(QIcon::fromTheme("help-faq"));
    ui->actionAboutQt->setIcon(QIcon::fromTheme("help-about"));

    // Ensure the application quits when the main window closes
    setAttribute(Qt::WA_QuitOnClose, true);
    QApplication::setQuitOnLastWindowClosed(true);

    setupWidgets();
    setupActions();
    setupStatusBar();
    setupSystemTray();
    setupGlobalShortcuts();
    loadRecentFiles();
    updateRecentFilesMenu();
    updateUI();
    updateWindowTitle();

    spdlog::info("MainWindow: Initialized");
}

MainWindow::~MainWindow()
{
    // Stop global shortcut monitoring
    if (m_globalShortcutTimer)
    {
        m_globalShortcutTimer->stop();
    }

    delete ui;
    spdlog::info("MainWindow: Destroyed");
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    spdlog::info("MainWindow: closeEvent triggered");

    if (m_modified)
    {
        // In test environment, automatically discard changes to avoid hanging
        if (TestUtils::isTestEnvironment())
        {
            spdlog::info(
              "MainWindow: Test environment - discarding unsaved changes"
            );
            // Continue with closing, no user interaction needed
        }
        else
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

    // Save recent files before shutdown
    saveRecentFiles();

    // Call application shutdown to clean up properly
    m_app.shutdown();

    spdlog::info("MainWindow: Application shutdown completed");
    event->accept();
}

void MainWindow::setupWidgets()
{
    // Create and setup custom widgets
    m_recordingWidget = new RecordingWidget(this);
    m_playbackWidget = new PlaybackWidget(m_app, this);

    // Replace the placeholder widgets in the tabs
    ui->recordingLayout->addWidget(m_recordingWidget);
    ui->playbackLayout->addWidget(m_playbackWidget);

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

    // Connect file loading signals
    connect(
      this,
      &MainWindow::fileLoadRequested,
      m_playbackWidget,
      &PlaybackWidget::loadFileRequested
    );
    connect(
      m_playbackWidget,
      &PlaybackWidget::fileLoaded,
      this,
      &MainWindow::onFileLoaded
    );
}

void MainWindow::setupActions()
{
    // File menu
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::onNewFile);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::onSaveFile);
    connect(
      ui->actionSaveAs, &QAction::triggered, this, &MainWindow::onSaveAsFile
    );
    connect(
      ui->actionRecentFiles,
      &QAction::triggered,
      this,
      &MainWindow::onRecentFiles
    );
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::onExit);

    // Edit menu
    connect(ui->actionClear, &QAction::triggered, this, &MainWindow::onClear);
    connect(
      ui->actionPreferences,
      &QAction::triggered,
      this,
      &MainWindow::onPreferences
    );

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
      ui->actionStopPlayback,
      &QAction::triggered,
      this,
      &MainWindow::onStopPlayback
    );

    // Set up global keyboard shortcuts
    setupKeyboardShortcuts();
}

void MainWindow::setupKeyboardShortcuts()
{
    const auto& config = m_app.getConfiguration();

    // Start Recording shortcut
    QString startRecordingKey = QString::fromStdString(
      config.getString("shortcuts.start_recording", "Ctrl+R")
    );
    m_startRecordingShortcut =
      new QShortcut(QKeySequence(startRecordingKey), this);
    connect(
      m_startRecordingShortcut,
      &QShortcut::activated,
      this,
      &MainWindow::onStartRecording
    );

    // Stop Recording shortcut
    QString stopRecordingKey = QString::fromStdString(
      config.getString("shortcuts.stop_recording", "Ctrl+Shift+R")
    );
    m_stopRecordingShortcut =
      new QShortcut(QKeySequence(stopRecordingKey), this);
    connect(
      m_stopRecordingShortcut,
      &QShortcut::activated,
      this,
      &MainWindow::onStopRecording
    );

    // Start Playback shortcut
    QString startPlaybackKey = QString::fromStdString(
      config.getString("shortcuts.start_playback", "Ctrl+P")
    );
    m_startPlaybackShortcut =
      new QShortcut(QKeySequence(startPlaybackKey), this);
    connect(
      m_startPlaybackShortcut,
      &QShortcut::activated,
      this,
      &MainWindow::onStartPlayback
    );

    // Stop Playback shortcut
    QString stopPlaybackKey = QString::fromStdString(
      config.getString("shortcuts.stop_playback", "Ctrl+Shift+P")
    );
    m_stopPlaybackShortcut = new QShortcut(QKeySequence(stopPlaybackKey), this);
    connect(
      m_stopPlaybackShortcut,
      &QShortcut::activated,
      this,
      &MainWindow::onStopPlayback
    );

    spdlog::info("MainWindow: Global keyboard shortcuts initialized");
}

void MainWindow::setupGlobalShortcuts()
{
    // Set up global shortcut monitoring timer
    m_globalShortcutTimer = new QTimer(this);
    connect(
      m_globalShortcutTimer,
      &QTimer::timeout,
      this,
      &MainWindow::checkGlobalShortcuts
    );
    m_globalShortcutTimer->start(
      50
    ); // Check every 50ms for responsive shortcuts

    spdlog::info("MainWindow: Global shortcut monitoring started");
}

void MainWindow::checkGlobalShortcuts()
{
#ifdef __linux__
    // Check for Ctrl+R (Start Recording) - XK_Control_L + XK_r
    bool ctrlPressed = isKeyPressed(XK_Control_L) || isKeyPressed(XK_Control_R);
    bool rPressed = isKeyPressed(XK_r);
    bool shiftPressed = isKeyPressed(XK_Shift_L) || isKeyPressed(XK_Shift_R);

    // Ctrl+R (start recording)
    if (ctrlPressed && rPressed && !shiftPressed)
    {
        if (!m_startRecordingKeyPressed)
        {
            m_startRecordingKeyPressed = true;

            // Only start if not already recording and not playing back
            if (!m_app.getEventRecorder().isRecording() &&
                m_app.getEventPlayer().getState() !=
                  Core::PlaybackState::Playing)
            {
                QTimer::singleShot(0, this, &MainWindow::onStartRecording);
                spdlog::info(
                  "MainWindow: Global shortcut triggered - Start Recording"
                );
            }
        }
    }
    else
    {
        m_startRecordingKeyPressed = false;
    }

    // Ctrl+Shift+R (stop recording)
    if (ctrlPressed && rPressed && shiftPressed)
    {
        if (!m_stopRecordingKeyPressed)
        {
            m_stopRecordingKeyPressed = true;

            // Only stop if currently recording
            if (m_app.getEventRecorder().isRecording())
            {
                QTimer::singleShot(0, this, &MainWindow::onStopRecording);
                spdlog::info(
                  "MainWindow: Global shortcut triggered - Stop Recording"
                );

                // Also restore window if minimized to tray
                if (!isVisible())
                {
                    QTimer::singleShot(100, this, &MainWindow::restoreFromTray);
                }
            }
        }
    }
    else
    {
        m_stopRecordingKeyPressed = false;
    }
#endif
}

bool MainWindow::isKeyPressed(int keyCode)
{
#ifdef __linux__
    Display* display = XOpenDisplay(nullptr);
    if (!display)
        return false;

    KeyCode kc = XKeysymToKeycode(display, keyCode);
    if (kc == NoSymbol)
    {
        XCloseDisplay(display);
        return false;
    }

    char keys[32];
    XQueryKeymap(display, keys);
    bool pressed = (keys[kc / 8] & (1 << (kc % 8))) != 0;

    XCloseDisplay(display);
    return pressed;
#else
    return false;
#endif
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
    if (m_modified)
    {
        // In test environment, automatically discard changes to avoid hanging
        if (TestUtils::isTestEnvironment())
        {
            spdlog::info(
              "MainWindow: Test environment - discarding unsaved changes for "
              "new file"
            );
            // Continue with creating new file, no user interaction needed
        }
        else
        {
            QMessageBox::StandardButton reply = QMessageBox::question(
              this,
              "Unsaved Changes",
              "You have unsaved changes. Do you want to save before creating a "
              "new "
              "file?",
              QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
            );

            if (reply == QMessageBox::Save)
            {
                onSaveFile();
            }
            else if (reply == QMessageBox::Cancel)
            {
                return;
            }
        }
    }

    // Clear current recording/playback
    std::lock_guard<std::mutex> lock(m_eventsMutex);
    m_recordedEvents.clear();

    if (m_recordingWidget)
    {
        m_recordingWidget->clearEvents();
    }

    m_currentFile.clear();
    m_modified = false;
    updateWindowTitle();
    updateUI();
    updateRecordingStatistics();

    ui->statusLabel->setText("New session started");
}

void MainWindow::onSaveFile()
{
    if (m_currentFile.isEmpty())
    {
        onSaveAsFile();
    }
    else
    {
        if (saveEventsToFile(m_currentFile))
        {
            m_modified = false;
            updateWindowTitle();
            ui->statusLabel->setText("File saved");
        }
        else
        {
            showErrorMessage(
              "Save Error", "Failed to save file: " + m_currentFile
            );
        }
    }
}

void MainWindow::onSaveAsFile()
{
    QString selectedFilter;
    QString fileName;

    // In test environment, use a temporary file path to avoid hanging on dialog
    if (TestUtils::isTestEnvironment())
    {
        fileName = QDir::temp().absoluteFilePath(
          "test_recording_" +
          QString::number(QCoreApplication::applicationPid()) + ".json"
        );
        selectedFilter = "JSON Files (*.json)";
        spdlog::info(
          "MainWindow: Test environment - using temp file: {}",
          fileName.toStdString()
        );
    }
    else
    {
        fileName = QFileDialog::getSaveFileName(
          this,
          "Save Recording File",
          m_currentFile.isEmpty() ? "" : m_currentFile,
          QString::fromStdString(
            Storage::EventStorageFactory::getFileDialogFilter()
          ),
          &selectedFilter
        );
    }

    if (!fileName.isEmpty())
    {
        if (saveEventsToFile(fileName))
        {
            m_currentFile = fileName;
            addToRecentFiles(fileName);
            m_modified = false;
            updateWindowTitle();
            ui->statusLabel->setText(
              "File saved: " + QFileInfo(fileName).fileName()
            );
        }
        else
        {
            showErrorMessage("Save Error", "Failed to save file: " + fileName);
        }
    }
}

void MainWindow::onExit()
{
    close();
}

void MainWindow::onAbout()
{
    // Skip showing About dialog in test environment
    if (TestUtils::isTestEnvironment())
    {
        spdlog::info("MainWindow: Skipping About dialog in test environment");
        return;
    }

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
    // Skip showing About Qt dialog in test environment
    if (TestUtils::isTestEnvironment())
    {
        spdlog::info(
          "MainWindow: Skipping About Qt dialog in test environment"
        );
        return;
    }

    QMessageBox::aboutQt(this);
}

void MainWindow::onRecentFiles()
{
    // This slot is called when the "Recent Files" menu item is clicked
    // The actual recent files are handled by dynamic menu items
    // This is just a placeholder - the actual file opening is handled
    // by lambda functions connected to dynamically created actions
}

void MainWindow::onClear()
{
    if (m_app.getEventRecorder().isRecording())
    {
        showWarningMessage(
          "Clear Events", "Cannot clear events while recording is in progress."
        );
        return;
    }

    if (m_recordedEvents.empty())
    {
        showInfoMessage("Clear Events", "No events to clear.");
        return;
    }

    // In test environment, automatically clear events without confirmation
    bool shouldClear = true;
    if (!TestUtils::isTestEnvironment())
    {
        QMessageBox::StandardButton reply = QMessageBox::question(
          this,
          "Clear Events",
          "Are you sure you want to clear all recorded events? This action "
          "cannot "
          "be undone.",
          QMessageBox::Yes | QMessageBox::No,
          QMessageBox::No
        );
        shouldClear = (reply == QMessageBox::Yes);
    }

    if (shouldClear)
    {
        std::lock_guard<std::mutex> lock(m_eventsMutex);
        m_recordedEvents.clear();

        if (m_recordingWidget)
        {
            m_recordingWidget->clearEvents();
        }

        m_modified = true;
        updateWindowTitle();
        updateRecordingStatistics();
        ui->statusLabel->setText("Events cleared");
    }
}

void MainWindow::onPreferences()
{
    // Check if we're in a test environment by checking various indicators
    bool inTestMode = false;

    // Try multiple ways to detect test environment
    if (qEnvironmentVariableIsSet("QT_FORCE_STDERR_LOGGING") ||
        qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
    {
        inTestMode = true;
    }

    // Also check application arguments for test-related flags
    const QStringList args = QCoreApplication::arguments();
    for (const QString& arg : args)
    {
        if (arg.contains("test", Qt::CaseInsensitive) ||
            arg.contains("unittest", Qt::CaseInsensitive))
        {
            inTestMode = true;
            break;
        }
    }

    if (inTestMode)
    {
        ui->statusLabel->setText(
          "Preferences dialog would open here (test mode)"
        );
        return;
    }

    // Create a preferences dialog with the configuration widget
    QDialog preferencesDialog(this);
    preferencesDialog.setWindowTitle("Preferences");
    preferencesDialog.setModal(true);
    preferencesDialog.resize(600, 400);

    // Create layout for the dialog
    QVBoxLayout* dialogLayout = new QVBoxLayout(&preferencesDialog);

    // Create configuration widget instance for the dialog
    ConfigurationWidget* configWidget =
      new ConfigurationWidget(m_app, &preferencesDialog);
    dialogLayout->addWidget(configWidget);

    // Add buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
      Qt::Horizontal,
      &preferencesDialog
    );
    dialogLayout->addWidget(buttonBox);

    connect(
      buttonBox,
      &QDialogButtonBox::accepted,
      &preferencesDialog,
      &QDialog::accept
    );
    connect(
      buttonBox,
      &QDialogButtonBox::rejected,
      &preferencesDialog,
      &QDialog::reject
    );

    // Show the dialog
    if (preferencesDialog.exec() == QDialog::Accepted)
    {
        ui->statusLabel->setText("Preferences saved");
    }
}

void MainWindow::onStartRecording()
{
    // Check if already recording to prevent duplicate operations
    if (m_app.getEventRecorder().isRecording())
    {
        spdlog::info(
          "MainWindow: Recording is already active, ignoring start request"
        );
        return;
    }

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
        // Clear the recording widget display and update UI state
        if (m_recordingWidget)
        {
            m_recordingWidget->clearEvents();
            m_recordingWidget->startRecordingUI();
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

            // Update widget UI state to show recording stopped
            if (m_recordingWidget)
            {
                m_recordingWidget->stopRecordingUI();
            }

            ui->statusLabel->setText("Recording stopped");
            updateUI();

            // Call the handler that updates all UI components
            onRecordingStopped();
        }
        else
        {
            spdlog::info(
              "MainWindow: Recording is not active, ignoring stop request"
            );
            ui->statusLabel->setText("Recording was not active");
        }
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

    // Auto-minimize to tray if enabled
    if (shouldAutoMinimize())
    {
        spdlog::info("MainWindow: Auto-minimizing to tray on recording start");
        QTimer::singleShot(
          500, this, &MainWindow::minimizeToTray
        ); // Small delay for UI update
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

    // Auto-restore from tray if it was auto-minimized
    if (!isVisible())
    {
        bool shouldRestore =
          shouldAutoMinimize() ||
          (m_trayIcon && m_trayIcon->isVisible() && m_wasVisibleBeforeMinimize);
        if (shouldRestore)
        {
            spdlog::info(
              "MainWindow: Auto-restoring from tray on recording stop"
            );
            QTimer::singleShot(
              200, this, &MainWindow::restoreFromTray
            ); // Small delay
        }
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

void MainWindow::onFileLoaded(const QString& filename)
{
    // Switch to playback tab when file is loaded
    ui->tabWidget->setCurrentWidget(ui->playbackTab);

    // Add to recent files
    addToRecentFiles(filename);

    // Update status
    ui->statusLabel->setText("File loaded: " + QFileInfo(filename).fileName());

    updateUI();
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

void MainWindow::showErrorMessage(const QString& title, const QString& message)
{
    if (!TestUtils::isTestEnvironment())
    {
        QMessageBox::critical(this, title, message);
    }
}

void MainWindow::showWarningMessage(
  const QString& title, const QString& message
)
{
    if (!TestUtils::isTestEnvironment())
    {
        QMessageBox::warning(this, title, message);
    }
}

void MainWindow::showInfoMessage(const QString& title, const QString& message)
{
    if (!TestUtils::isTestEnvironment())
    {
        QMessageBox::information(this, title, message);
    }
}

void MainWindow::setupSystemTray()
{
    // Check if system tray is available
    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        spdlog::warn("MainWindow: System tray not available on this system");
        return;
    }

    // Create system tray icon
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/icons/app.png")); // Use application icon
    m_trayIcon->setToolTip("MouseRecorder");

    // Create tray menu
    m_trayMenu = new QMenu(this);

    m_showHideAction = m_trayMenu->addAction("Show/Hide");
    connect(
      m_showHideAction, &QAction::triggered, this, &MainWindow::onShowHideAction
    );

    m_trayMenu->addSeparator();

    m_exitAction = m_trayMenu->addAction("Exit");
    connect(
      m_exitAction, &QAction::triggered, this, &MainWindow::onTrayExitAction
    );

    m_trayIcon->setContextMenu(m_trayMenu);

    // Connect tray icon activation
    connect(
      m_trayIcon,
      &QSystemTrayIcon::activated,
      this,
      &MainWindow::onTrayIconActivated
    );

    // Show the tray icon
    m_trayIcon->show();

    spdlog::info("MainWindow: System tray initialized");
}

void MainWindow::minimizeToTray()
{
    if (m_trayIcon && m_trayIcon->isVisible())
    {
        m_wasVisibleBeforeMinimize = isVisible();
        hide();
        spdlog::info("MainWindow: Minimized to system tray");

        // Show balloon message on first minimize during recording
        static bool firstTimeMinimized = true;
        if (firstTimeMinimized && m_app.getEventRecorder().isRecording())
        {
            m_trayIcon->showMessage(
              "MouseRecorder",
              "Recording in background. Double-click tray icon to restore.",
              QSystemTrayIcon::Information,
              3000
            );
            firstTimeMinimized = false;
        }
    }
    else
    {
        showMinimized();
        spdlog::debug(
          "MainWindow: System tray not available, minimized to taskbar"
        );
    }
}

void MainWindow::restoreFromTray()
{
    // Always try to restore if window is hidden and tray icon exists
    if (!isVisible() && m_trayIcon && m_trayIcon->isVisible())
    {
        show();
        raise();
        activateWindow();
        setWindowState(
          (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive
        );
        spdlog::info("MainWindow: Restored from system tray");
    }
    else if (m_wasVisibleBeforeMinimize)
    {
        // Legacy path for cases where window state tracking worked
        show();
        raise();
        activateWindow();
        spdlog::info("MainWindow: Restored from system tray (legacy path)");
    }
    else
    {
        spdlog::debug(
          "MainWindow: Window already visible or tray not available"
        );
    }
}

bool MainWindow::shouldAutoMinimize() const
{
    return m_app.getConfiguration().getBool("ui.auto_minimize_on_record", true);
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
    {
    case QSystemTrayIcon::DoubleClick:
    case QSystemTrayIcon::Trigger:
        onShowHideAction();
        break;
    default:
        break;
    }
}

void MainWindow::onShowHideAction()
{
    if (isVisible())
    {
        minimizeToTray();
    }
    else
    {
        restoreFromTray();
    }
}

void MainWindow::onTrayExitAction()
{
    // Ensure proper shutdown
    close();
}

void MainWindow::changeEvent(QEvent* event)
{
    QMainWindow::changeEvent(event);

    if (event->type() == QEvent::WindowStateChange)
    {
        // If minimized and system tray is available, hide to tray
        if (isMinimized() && m_trayIcon && m_trayIcon->isVisible())
        {
            QTimer::singleShot(0, this, &MainWindow::minimizeToTray);
        }
    }
}

void MainWindow::updateRecentFilesMenu()
{
    if (!m_recentFilesMenu)
    {
        m_recentFilesMenu = new QMenu("Recent Files", this);
        ui->actionRecentFiles->setMenu(m_recentFilesMenu);
    }
    else
    {
        m_recentFilesMenu->clear();
    }

    for (int i = 0; i < m_recentFiles.size() && i < MainWindow::MaxRecentFiles;
         ++i)
    {
        const QString& filePath = m_recentFiles.at(i);
        QString displayName =
          QString("&%1 %2").arg(i + 1).arg(QFileInfo(filePath).fileName());

        QAction* action = m_recentFilesMenu->addAction(displayName);
        action->setToolTip(filePath);

        // Connect the action to emit the fileLoadRequested signal
        connect(
          action,
          &QAction::triggered,
          this,
          [this, filePath]()
          {
              // Check if file exists before attempting to load
              QFileInfo fileInfo(filePath);
              if (!fileInfo.exists())
              {
                  // Remove file from recent files if it doesn't exist
                  m_recentFiles.removeAll(filePath);
                  updateRecentFilesMenu();
                  saveRecentFiles();
                  showErrorMessage(
                    "Load Error",
                    "File not found: " + filePath +
                      "\n\nFile has been removed from recent files."
                  );
                  return;
              }

              // Emit signal to request file loading - this will be handled
              // by PlaybackWidget asynchronously
              emit fileLoadRequested(filePath);
          }
        );
    }

    if (m_recentFiles.isEmpty())
    {
        QAction* noRecentAction =
          m_recentFilesMenu->addAction("No recent files");
        noRecentAction->setEnabled(false);
    }
    else
    {
        m_recentFilesMenu->addSeparator();
        QAction* clearAction =
          m_recentFilesMenu->addAction("Clear Recent Files");
        connect(
          clearAction,
          &QAction::triggered,
          this,
          [this]()
          {
              m_recentFiles.clear();
              updateRecentFilesMenu();
              saveRecentFiles();
          }
        );
    }
}

void MainWindow::addToRecentFiles(const QString& filename)
{
    QString canonicalFilename = QFileInfo(filename).canonicalFilePath();

    // Remove if already exists to move it to the top
    m_recentFiles.removeAll(canonicalFilename);

    // Add to the beginning
    m_recentFiles.prepend(canonicalFilename);

    // Keep only the specified number of recent files
    while (m_recentFiles.size() > MainWindow::MaxRecentFiles)
    {
        m_recentFiles.removeLast();
    }

    updateRecentFilesMenu();
    saveRecentFiles();
}

void MainWindow::loadRecentFiles()
{
    const auto& config = m_app.getConfiguration();
    QStringList recentFilesList =
      QString::fromStdString(config.getString("ui.recent_files", ""))
        .split(";", Qt::SkipEmptyParts);

    // Filter out files that no longer exist
    for (const QString& file : recentFilesList)
    {
        if (QFileInfo::exists(file))
        {
            m_recentFiles.append(file);
        }
    }

    // Keep only the specified number
    while (m_recentFiles.size() > MainWindow::MaxRecentFiles)
    {
        m_recentFiles.removeLast();
    }
}

void MainWindow::saveRecentFiles()
{
    QString recentFilesString = m_recentFiles.join(";");
    auto& config = const_cast<Core::IConfiguration&>(m_app.getConfiguration());
    config.setString("ui.recent_files", recentFilesString.toStdString());
    // The configuration is automatically saved by the MouseRecorderApp
}

bool MainWindow::saveEventsToFile(const QString& filename)
{
    try
    {
        if (m_recordedEvents.empty())
        {
            showWarningMessage(
              "Save File", "No events to save. Please record some events first."
            );
            return false;
        }

        auto storage = Storage::EventStorageFactory::createStorageFromFilename(
          filename.toStdString()
        );

        if (!storage)
        {
            spdlog::error(
              "MainWindow: Unsupported file format for: {}",
              filename.toStdString()
            );
            return false;
        }

        // Create a copy of events for saving
        std::vector<std::unique_ptr<Core::Event>> eventsToSave;
        {
            std::lock_guard<std::mutex> lock(m_eventsMutex);
            for (const auto& event : m_recordedEvents)
            {
                if (event)
                {
                    eventsToSave.emplace_back(
                      std::make_unique<Core::Event>(*event)
                    );
                }
            }
        }

        // Ensure the file has the correct extension
        std::string fileWithExtension = filename.toStdString();
        if (std::filesystem::path(filename.toStdString()).extension() !=
            storage->getFileExtension())
        {
            fileWithExtension += storage->getFileExtension();
        }

        if (storage->saveEvents(eventsToSave, fileWithExtension))
        {
            spdlog::info(
              "MainWindow: Saved {} events to {}",
              eventsToSave.size(),
              fileWithExtension
            );
            return true;
        }
        else
        {
            spdlog::error(
              "MainWindow: Failed to save events to {}: {}",
              fileWithExtension,
              storage->getLastError()
            );
            return false;
        }
    }
    catch (const std::exception& e)
    {
        spdlog::error(
          "MainWindow: Exception saving events to {}: {}",
          filename.toStdString(),
          e.what()
        );
        return false;
    }
}

} // namespace MouseRecorder::GUI

