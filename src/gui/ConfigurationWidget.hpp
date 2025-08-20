// Copyright (c) 2025 JackLee
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include <QWidget>

namespace Ui
{
class ConfigurationWidget;
}

namespace MouseRecorder::Application
{
class MouseRecorderApp;
}

namespace MouseRecorder::GUI
{

/**
 * @brief Widget for application configuration
 */
class ConfigurationWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit ConfigurationWidget(Application::MouseRecorderApp& app,
                                 QWidget* parent = nullptr);
    ~ConfigurationWidget();

  public slots:
    void loadConfiguration();
    void saveConfiguration();

  private slots:
    void onRestoreDefaults();
    void onApply();
    void onBrowseLogFile();

  private:
    void setupUI();
    void updateUI();

  private:
    Ui::ConfigurationWidget* ui;
    Application::MouseRecorderApp& m_app;
};

} // namespace MouseRecorder::GUI
