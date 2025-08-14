#pragma once

#include <QCoreApplication>
#include <QString>

namespace MouseRecorder::GUI
{

/**
 * @brief Utility functions for handling test environments
 */
class TestUtils
{
  public:
    /**
     * @brief Check if we are running in a test environment
     * @return true if running tests, false otherwise
     */
    static bool isTestEnvironment()
    {
        // Check if the application name indicates we're running tests
        auto app = QCoreApplication::instance();
        if (!app)
        {
            return false;
        }

        QString appName = app->applicationName();
        QString executableName = app->applicationFilePath();

        // Check various indicators that we're in a test environment
        return appName.contains("Test", Qt::CaseInsensitive) ||
               executableName.contains("Test", Qt::CaseInsensitive) ||
               executableName.contains("ctest", Qt::CaseInsensitive);
    }
};

} // namespace MouseRecorder::GUI
