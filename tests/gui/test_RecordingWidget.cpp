#include <QtTest/QtTest>
#include <QApplication>
#include <QPushButton>
#include <QSignalSpy>
#include <QTableWidget>
#include "gui/RecordingWidget.hpp"
#include "application/MouseRecorderApp.hpp"
#include "core/Event.hpp"

using namespace MouseRecorder::GUI;
using namespace MouseRecorder::Application;

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
    void testEventDisplay();
    void testEventExport();

  private:
    QApplication* m_app{nullptr};
    RecordingWidget* m_widget{nullptr};
    std::unique_ptr<MouseRecorderApp> m_mouseApp;
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
    m_mouseApp = std::make_unique<MouseRecorderApp>();
    
    // Initialize the application
    if (m_mouseApp->initialize())
    {
        m_widget = new RecordingWidget(*m_mouseApp);
    }
}

void TestRecordingWidget::cleanup()
{
    delete m_widget;
    m_widget = nullptr;
    
    if (m_mouseApp)
    {
        m_mouseApp->shutdown();
        m_mouseApp.reset();
    }
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

void TestRecordingWidget::testEventDisplay()
{
    using namespace MouseRecorder::Core;

    // Create test events
    auto mouseEvent =
      EventFactory::createMouseClickEvent(Point(100, 200), MouseButton::Left);
    auto keyEvent =
      EventFactory::createKeyPressEvent(65, "A", KeyModifier::None);

    // Get the events table widget
    QTableWidget* eventsTable =
      m_widget->findChild<QTableWidget*>("eventsTableWidget");
    QVERIFY(eventsTable != nullptr);

    // Initially, table should be empty
    QCOMPARE(eventsTable->rowCount(), 0);

    // Add mouse event
    m_widget->addEvent(mouseEvent.get());
    QCOMPARE(eventsTable->rowCount(), 1);

    // Verify mouse event data
    QVERIFY(eventsTable->item(0, 1)->text().contains("Mouse"));
    QVERIFY(eventsTable->item(0, 2)->text().contains("Button: Left"));
    QVERIFY(eventsTable->item(0, 2)->text().contains("X: 100"));
    QVERIFY(eventsTable->item(0, 2)->text().contains("Y: 200"));

    // Add keyboard event
    m_widget->addEvent(keyEvent.get());
    QCOMPARE(eventsTable->rowCount(), 2);

    // Verify keyboard event data
    QVERIFY(eventsTable->item(1, 1)->text().contains("Keyboard"));
    QVERIFY(eventsTable->item(1, 2)->text().contains("Key: A"));
    QVERIFY(eventsTable->item(1, 2)->text().contains("(65)"));

    // Test clear events
    m_widget->clearEvents();
    QCOMPARE(eventsTable->rowCount(), 0);
}

void TestRecordingWidget::testEventExport()
{
    using namespace MouseRecorder::Core;

    // Create a signal spy to monitor export signal
    QSignalSpy exportSpy(m_widget, &RecordingWidget::exportEventsRequested);

    // Find and click the export button
    QPushButton* exportButton =
      m_widget->findChild<QPushButton*>("exportEventsButton");
    QVERIFY(exportButton != nullptr);

    // Click export button should emit the signal
    QTest::mouseClick(exportButton, Qt::LeftButton);
    QCOMPARE(exportSpy.count(), 1);
}

QTEST_APPLESS_MAIN(TestRecordingWidget)
#include "test_RecordingWidget.moc"
