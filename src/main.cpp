#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QCommandLineParser>
#include <QMessageBox>
#include <spdlog/spdlog.h>

#include "application/MouseRecorderApp.hpp"
#include "gui/MainWindow.hpp"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // Set application properties
    app.setApplicationName("MouseRecorder");
    app.setApplicationVersion("1.0.0");
    app.setApplicationDisplayName(
      "MouseRecorder - Event Recording and Playback"
    );
    app.setOrganizationName("MouseRecorder Team");
    app.setOrganizationDomain("mouserecorder.org");

    // Setup command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription(
      "Cross-platform Mouse and Keyboard Event Recorder"
    );
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption configOption(
      QStringList() << "c" << "config", "Configuration file path", "config_file"
    );
    parser.addOption(configOption);

    QCommandLineOption logLevelOption(
      QStringList() << "l" << "log-level",
      "Set log level (trace, debug, info, warn, error, critical, off)",
      "level",
      "info"
    );
    parser.addOption(logLevelOption);

    parser.process(app);

    // Get configuration file path
    QString configFile = parser.value(configOption);
    if (configFile.isEmpty())
    {
        // Use default location
        QString configDir =
          QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
        QDir().mkpath(configDir);
        configFile = QDir(configDir).filePath("mouserecorder.conf");
    }

    // Initialize the application core
    MouseRecorder::Application::MouseRecorderApp mouseRecorderApp;

    if (!mouseRecorderApp.initialize(configFile.toStdString()))
    {
        QMessageBox::critical(
          nullptr,
          "Initialization Error",
          QString("Failed to initialize application: %1")
            .arg(QString::fromStdString(mouseRecorderApp.getLastError()))
        );
        return 1;
    }

    // Create and show main window
    MouseRecorder::GUI::MainWindow mainWindow(mouseRecorderApp);

    // Apply theme if specified in configuration
    auto& config = mouseRecorderApp.getConfiguration();
    QString theme =
      QString::fromStdString(config.getString("ui.theme", "system"));

    if (theme != "system")
    {
        QStringList availableStyles = QStyleFactory::keys();
        if (availableStyles.contains(theme, Qt::CaseInsensitive))
        {
            app.setStyle(QStyleFactory::create(theme));
        }
    }

    // Restore window geometry
    int windowWidth = config.getInt("ui.window_width", 900);
    int windowHeight = config.getInt("ui.window_height", 700);
    int windowX = config.getInt("ui.window_x", 100);
    int windowY = config.getInt("ui.window_y", 100);
    bool windowMaximized = config.getBool("ui.window_maximized", false);

    mainWindow.resize(windowWidth, windowHeight);
    mainWindow.move(windowX, windowY);

    if (windowMaximized)
    {
        mainWindow.showMaximized();
    }
    else
    {
        mainWindow.show();
    }

    spdlog::info("MouseRecorder application started successfully");

    // Run the application
    int result = app.exec();

    spdlog::info("MouseRecorder application exiting with code {}", result);
    return result;
}
