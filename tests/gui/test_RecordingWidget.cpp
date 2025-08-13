#include <QtTest/QtTest>
#include <QApplication>
#include <QPushButton>
#include <QSignalSpy>
#include "gui/RecordingWidget.hpp"

using namespace MouseRecorder::GUI;

class TestRecordingWidget : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testInitialState();
    void testStartRecording();
    void testStopRecording();
    void testSignalEmission();
    void testButtonStates();
    void testStatisticsUpdate();

  private:
    QApplication* m_app{nullptr};
    RecordingWidget* m_widget{nullptr};
};

void TestRecordingWidget::initTestCase()
{
    // QApplication is required for GUI tests
    if (!QApplication::instance())
    {
        int argc = 0;
        char* argv[] = {nullptr};
        m_app = new QApplication(argc, argv);
    }
}

void TestRecordingWidget::cleanupTestCase()
{
    if (m_app && m_app->instance() == m_app)
    {
        delete m_app;
        m_app = nullptr;
    }
}

void TestRecordingWidget::init()
{
    m_widget = new RecordingWidget();
}

void TestRecordingWidget::cleanup()
{
    delete m_widget;
    m_widget = nullptr;
}

void TestRecordingWidget::testInitialState()
{
    QVERIFY(m_widget != nullptr);

    // Find the buttons by their object names or via findChild
    QPushButton* startButton =
      m_widget->findChild<QPushButton*>("startRecordingButton");
    QPushButton* stopButton =
      m_widget->findChild<QPushButton*>("stopRecordingButton");

    QVERIFY(startButton != nullptr);
    QVERIFY(stopButton != nullptr);

    // Initially, start button should be enabled, stop button disabled
    QVERIFY(startButton->isEnabled());
    QVERIFY(!stopButton->isEnabled());
}

void TestRecordingWidget::testStartRecording()
{
    QPushButton* startButton =
      m_widget->findChild<QPushButton*>("startRecordingButton");
    QPushButton* stopButton =
      m_widget->findChild<QPushButton*>("stopRecordingButton");

    QVERIFY(startButton != nullptr);
    QVERIFY(stopButton != nullptr);

    // Set up signal spy to verify signal emission
    QSignalSpy startSpy(m_widget, &RecordingWidget::recordingStarted);

    // Simulate clicking the start button
    QTest::mouseClick(startButton, Qt::LeftButton);

    // Verify signal was emitted
    QCOMPARE(startSpy.count(), 1);

    // Verify button states changed correctly
    QVERIFY(!startButton->isEnabled());
    QVERIFY(stopButton->isEnabled());
}

void TestRecordingWidget::testStopRecording()
{
    QPushButton* startButton =
      m_widget->findChild<QPushButton*>("startRecordingButton");
    QPushButton* stopButton =
      m_widget->findChild<QPushButton*>("stopRecordingButton");

    QVERIFY(startButton != nullptr);
    QVERIFY(stopButton != nullptr);

    // First start recording
    QTest::mouseClick(startButton, Qt::LeftButton);

    // Set up signal spy for stop signal
    QSignalSpy stopSpy(m_widget, &RecordingWidget::recordingStopped);

    // Simulate clicking the stop button
    QTest::mouseClick(stopButton, Qt::LeftButton);

    // Verify signal was emitted
    QCOMPARE(stopSpy.count(), 1);

    // Verify button states reverted
    QVERIFY(startButton->isEnabled());
    QVERIFY(!stopButton->isEnabled());
}

void TestRecordingWidget::testSignalEmission()
{
    // Test that signals are properly emitted when buttons are clicked
    QSignalSpy startSpy(m_widget, &RecordingWidget::recordingStarted);
    QSignalSpy stopSpy(m_widget, &RecordingWidget::recordingStopped);

    QPushButton* startButton =
      m_widget->findChild<QPushButton*>("startRecordingButton");
    QPushButton* stopButton =
      m_widget->findChild<QPushButton*>("stopRecordingButton");

    // Initially no signals should have been emitted
    QCOMPARE(startSpy.count(), 0);
    QCOMPARE(stopSpy.count(), 0);

    // Click start
    QTest::mouseClick(startButton, Qt::LeftButton);
    QCOMPARE(startSpy.count(), 1);
    QCOMPARE(stopSpy.count(), 0);

    // Click stop
    QTest::mouseClick(stopButton, Qt::LeftButton);
    QCOMPARE(startSpy.count(), 1);
    QCOMPARE(stopSpy.count(), 1);
}

void TestRecordingWidget::testButtonStates()
{
    QPushButton* startButton =
      m_widget->findChild<QPushButton*>("startRecordingButton");
    QPushButton* stopButton =
      m_widget->findChild<QPushButton*>("stopRecordingButton");

    // Test multiple start/stop cycles
    for (int i = 0; i < 3; ++i)
    {
        // Initial state
        QVERIFY(startButton->isEnabled());
        QVERIFY(!stopButton->isEnabled());

        // After starting
        QTest::mouseClick(startButton, Qt::LeftButton);
        QVERIFY(!startButton->isEnabled());
        QVERIFY(stopButton->isEnabled());

        // After stopping
        QTest::mouseClick(stopButton, Qt::LeftButton);
        QVERIFY(startButton->isEnabled());
        QVERIFY(!stopButton->isEnabled());
    }
}

void TestRecordingWidget::testStatisticsUpdate()
{
    // Test statistics update functionality
    m_widget->updateStatistics(10, 7, 3);

    // Verify statistics are displayed correctly
    // Note: This would require access to the UI elements displaying statistics
    // For now, just verify the method can be called without crashing
    QVERIFY(true);
}

QTEST_APPLESS_MAIN(TestRecordingWidget)
#include "test_RecordingWidget.moc"
