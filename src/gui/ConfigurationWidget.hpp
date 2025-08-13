#pragma once

#include <QWidget>

namespace Ui
{
class ConfigurationWidget;
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
    explicit ConfigurationWidget(QWidget* parent = nullptr);
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
};

} // namespace MouseRecorder::GUI
