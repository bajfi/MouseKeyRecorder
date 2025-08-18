#include <gtest/gtest.h>
#include "storage/JsonEventStorage.hpp"
#include "storage/XmlEventStorage.hpp"
#include "storage/BinaryEventStorage.hpp"
#include "storage/EventStorageFactory.hpp"
#include "core/Event.hpp"
#include <filesystem>
#include <fstream>

using namespace MouseRecorder::Storage;
using namespace MouseRecorder::Core;

class EventStorageTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create test events
        m_testEvents.push_back(EventFactory::createMouseMoveEvent({100, 200}));
        m_testEvents.push_back(
            EventFactory::createMouseClickEvent({150, 250}, MouseButton::Left));
        m_testEvents.push_back(
            EventFactory::createKeyPressEvent(65, "A", KeyModifier::Ctrl));
        m_testEvents.push_back(
            EventFactory::createMouseWheelEvent({200, 300}, 120));
        m_testEvents.push_back(EventFactory::createKeyReleaseEvent(65, "A"));

        // Test file names
        m_jsonFile = "test_events.json";
        m_xmlFile = "test_events.xml";
        m_binaryFile = "test_events.mre";
    }

    void TearDown() override
    {
        // Clean up test files
        std::vector<std::string> testFiles = {
            m_jsonFile, m_xmlFile, m_binaryFile};
        for (const auto& file : testFiles)
        {
            if (std::filesystem::exists(file))
            {
                std::filesystem::remove(file);
            }
        }
    }

    std::vector<std::unique_ptr<Event>> createEventCopies(
        const std::vector<std::unique_ptr<Event>>& original)
    {
        std::vector<std::unique_ptr<Event>> copies;
        for (const auto& event : original)
        {
            // Create a copy of the event
            copies.push_back(std::make_unique<Event>(*event));
        }
        return copies;
    }

    void verifyEventsEqual(const std::vector<std::unique_ptr<Event>>& events1,
                           const std::vector<std::unique_ptr<Event>>& events2)
    {
        ASSERT_EQ(events1.size(), events2.size());

        for (size_t i = 0; i < events1.size(); ++i)
        {
            const auto& event1 = events1[i];
            const auto& event2 = events2[i];

            EXPECT_EQ(event1->getType(), event2->getType());
            EXPECT_EQ(event1->getTimestampMs(), event2->getTimestampMs());

            if (event1->isMouseEvent())
            {
                const auto* mouse1 = event1->getMouseData();
                const auto* mouse2 = event2->getMouseData();
                ASSERT_NE(mouse1, nullptr);
                ASSERT_NE(mouse2, nullptr);

                EXPECT_EQ(mouse1->position.x, mouse2->position.x);
                EXPECT_EQ(mouse1->position.y, mouse2->position.y);
                EXPECT_EQ(mouse1->button, mouse2->button);
                EXPECT_EQ(mouse1->wheelDelta, mouse2->wheelDelta);
                EXPECT_EQ(mouse1->modifiers, mouse2->modifiers);
            }
            else if (event1->isKeyboardEvent())
            {
                const auto* key1 = event1->getKeyboardData();
                const auto* key2 = event2->getKeyboardData();
                ASSERT_NE(key1, nullptr);
                ASSERT_NE(key2, nullptr);

                EXPECT_EQ(key1->keyCode, key2->keyCode);
                EXPECT_EQ(key1->keyName, key2->keyName);
                EXPECT_EQ(key1->modifiers, key2->modifiers);
                EXPECT_EQ(key1->isRepeated, key2->isRepeated);
            }
        }
    }

    std::vector<std::unique_ptr<Event>> m_testEvents;
    std::string m_jsonFile;
    std::string m_xmlFile;
    std::string m_binaryFile;
};

TEST_F(EventStorageTest, JsonStorageSaveAndLoad)
{
    JsonEventStorage storage;

    // Save events
    auto eventCopies = createEventCopies(m_testEvents);
    EXPECT_TRUE(storage.saveEvents(eventCopies, m_jsonFile));
    EXPECT_TRUE(storage.getLastError().empty());

    // Verify file was created
    EXPECT_TRUE(std::filesystem::exists(m_jsonFile));

    // Load events
    std::vector<std::unique_ptr<Event>> loadedEvents;
    StorageMetadata metadata;
    EXPECT_TRUE(storage.loadEvents(m_jsonFile, loadedEvents, metadata));
    EXPECT_FALSE(loadedEvents.empty());
    EXPECT_TRUE(storage.getLastError().empty());

    // Verify events are the same
    verifyEventsEqual(m_testEvents, loadedEvents);
}

TEST_F(EventStorageTest, XmlStorageSaveAndLoad)
{
    XmlEventStorage storage;

    // Save events
    auto eventCopies = createEventCopies(m_testEvents);
    EXPECT_TRUE(storage.saveEvents(eventCopies, m_xmlFile));
    EXPECT_TRUE(storage.getLastError().empty());

    // Verify file was created
    EXPECT_TRUE(std::filesystem::exists(m_xmlFile));

    // Load events
    std::vector<std::unique_ptr<Event>> loadedEvents;
    StorageMetadata metadata;
    EXPECT_TRUE(storage.loadEvents(m_xmlFile, loadedEvents, metadata));
    EXPECT_FALSE(loadedEvents.empty());
    EXPECT_TRUE(storage.getLastError().empty());

    // Verify events are the same
    verifyEventsEqual(m_testEvents, loadedEvents);
}

TEST_F(EventStorageTest, BinaryStorageSaveAndLoad)
{
    BinaryEventStorage storage;

    // Save events
    auto eventCopies = createEventCopies(m_testEvents);
    EXPECT_TRUE(storage.saveEvents(eventCopies, m_binaryFile));
    EXPECT_TRUE(storage.getLastError().empty());

    // Verify file was created
    EXPECT_TRUE(std::filesystem::exists(m_binaryFile));

    // Load events
    std::vector<std::unique_ptr<Event>> loadedEvents;
    StorageMetadata metadata;
    EXPECT_TRUE(storage.loadEvents(m_binaryFile, loadedEvents, metadata));
    EXPECT_FALSE(loadedEvents.empty());
    EXPECT_TRUE(storage.getLastError().empty());

    // Verify events are the same
    verifyEventsEqual(m_testEvents, loadedEvents);
}

TEST_F(EventStorageTest, StorageFactory)
{
    // Test JSON storage creation
    auto jsonStorage = EventStorageFactory::createStorage(StorageFormat::Json);
    ASSERT_NE(jsonStorage, nullptr);

    auto eventCopies = createEventCopies(m_testEvents);
    EXPECT_TRUE(jsonStorage->saveEvents(eventCopies, m_jsonFile));

    // Test XML storage creation
    auto xmlStorage = EventStorageFactory::createStorage(StorageFormat::Xml);
    ASSERT_NE(xmlStorage, nullptr);

    eventCopies = createEventCopies(m_testEvents);
    EXPECT_TRUE(xmlStorage->saveEvents(eventCopies, m_xmlFile));

    // Test Binary storage creation
    auto binaryStorage =
        EventStorageFactory::createStorage(StorageFormat::Binary);
    ASSERT_NE(binaryStorage, nullptr);

    eventCopies = createEventCopies(m_testEvents);
    EXPECT_TRUE(binaryStorage->saveEvents(eventCopies, m_binaryFile));
}

TEST_F(EventStorageTest, EmptyEventsSave)
{
    JsonEventStorage storage;
    std::vector<std::unique_ptr<Event>> emptyEvents;

    EXPECT_TRUE(storage.saveEvents(emptyEvents, m_jsonFile));

    std::vector<std::unique_ptr<Event>> loadedEvents;
    StorageMetadata metadata;
    EXPECT_TRUE(storage.loadEvents(m_jsonFile, loadedEvents, metadata));
    EXPECT_TRUE(loadedEvents.empty());
}

TEST_F(EventStorageTest, InvalidFileLoad)
{
    JsonEventStorage storage;

    // Try to load non-existent file
    std::vector<std::unique_ptr<Event>> events;
    StorageMetadata metadata;
    EXPECT_FALSE(
        storage.loadEvents("non_existent_file.json", events, metadata));
    EXPECT_TRUE(events.empty());
    EXPECT_FALSE(storage.getLastError().empty());
}

TEST_F(EventStorageTest, CorruptedFileLoad)
{
    // Create a corrupted JSON file
    std::ofstream corruptedFile(m_jsonFile);
    corruptedFile << "{ this is not valid json }";
    corruptedFile.close();

    JsonEventStorage storage;
    std::vector<std::unique_ptr<Event>> events;
    StorageMetadata metadata;
    EXPECT_FALSE(storage.loadEvents(m_jsonFile, events, metadata));
    EXPECT_TRUE(events.empty());
    EXPECT_FALSE(storage.getLastError().empty());
}

TEST_F(EventStorageTest, SaveToInvalidPath)
{
    JsonEventStorage storage;
    auto eventCopies = createEventCopies(m_testEvents);

    // Try to save to invalid path
    EXPECT_FALSE(storage.saveEvents(eventCopies, "/invalid/path/file.json"));
    EXPECT_FALSE(storage.getLastError().empty());
}

TEST_F(EventStorageTest, FileFormatDetection)
{
    // This test would verify that storage can detect format from file extension
    // For now, we'll just test that the factory creates different types

    auto jsonStorage1 = EventStorageFactory::createStorage(StorageFormat::Json);
    auto jsonStorage2 = EventStorageFactory::createStorage(StorageFormat::Json);
    auto xmlStorage = EventStorageFactory::createStorage(StorageFormat::Xml);
    auto binaryStorage =
        EventStorageFactory::createStorage(StorageFormat::Binary);

    // Each call should create a new instance
    EXPECT_NE(jsonStorage1.get(), jsonStorage2.get());
    EXPECT_NE(jsonStorage1.get(), xmlStorage.get());
    EXPECT_NE(jsonStorage1.get(), binaryStorage.get());
}

TEST_F(EventStorageTest, RecordingWidgetExportIntegration)
{
    // This test simulates the export process from the RecordingWidget
    // by creating, displaying, and exporting events using the same workflow

    // Simulate events being recorded (as they would be passed to
    // RecordingWidget)
    std::vector<std::unique_ptr<Event>> recordedEvents;

    // Create events similar to what would be captured during recording
    recordedEvents.push_back(EventFactory::createMouseMoveEvent({10, 15}));
    recordedEvents.push_back(
        EventFactory::createMouseClickEvent({25, 35}, MouseButton::Left));
    recordedEvents.push_back(EventFactory::createMouseMoveEvent({50, 75}));
    recordedEvents.push_back(
        EventFactory::createKeyPressEvent(32, "Space", KeyModifier::None));
    recordedEvents.push_back(
        EventFactory::createMouseClickEvent({100, 125}, MouseButton::Right));
    recordedEvents.push_back(
        EventFactory::createKeyReleaseEvent(32, "Space", KeyModifier::None));

    ASSERT_EQ(recordedEvents.size(), 6);

    // Test export to JSON (most common format)
    std::string exportFile = "exported_recording.json";
    auto storage = EventStorageFactory::createStorageFromFilename(exportFile);
    ASSERT_NE(storage, nullptr);

    // Create copies for export (simulating what MainWindow::onExportEvents
    // does)
    std::vector<std::unique_ptr<Event>> eventsToExport;
    for (const auto& event : recordedEvents)
    {
        if (event)
        {
            eventsToExport.push_back(std::make_unique<Event>(*event));
        }
    }

    // Test the export process
    EXPECT_TRUE(storage->saveEvents(eventsToExport, exportFile));

    // Verify the exported file exists and contains the correct data
    EXPECT_TRUE(std::filesystem::exists(exportFile));

    // Load and verify the exported events
    std::vector<std::unique_ptr<Event>> loadedEvents;
    StorageMetadata metadata;
    EXPECT_TRUE(storage->loadEvents(exportFile, loadedEvents, metadata));

    // Verify event count and types match
    EXPECT_EQ(loadedEvents.size(), recordedEvents.size());

    // Verify specific events to ensure data integrity
    if (loadedEvents.size() >= 6)
    {
        // Check first mouse move
        EXPECT_EQ(loadedEvents[0]->getType(), EventType::MouseMove);
        auto mouseData0 = loadedEvents[0]->getMouseData();
        ASSERT_NE(mouseData0, nullptr);
        EXPECT_EQ(mouseData0->position.x, 10);
        EXPECT_EQ(mouseData0->position.y, 15);

        // Check first mouse click
        EXPECT_EQ(loadedEvents[1]->getType(), EventType::MouseClick);
        auto mouseData1 = loadedEvents[1]->getMouseData();
        ASSERT_NE(mouseData1, nullptr);
        EXPECT_EQ(mouseData1->position.x, 25);
        EXPECT_EQ(mouseData1->position.y, 35);
        EXPECT_EQ(mouseData1->button, MouseButton::Left);

        // Check keyboard events
        EXPECT_EQ(loadedEvents[3]->getType(), EventType::KeyPress);
        auto keyData3 = loadedEvents[3]->getKeyboardData();
        ASSERT_NE(keyData3, nullptr);
        EXPECT_EQ(keyData3->keyCode, 32u);
        EXPECT_EQ(keyData3->keyName, "Space");

        EXPECT_EQ(loadedEvents[5]->getType(), EventType::KeyRelease);
        auto keyData5 = loadedEvents[5]->getKeyboardData();
        ASSERT_NE(keyData5, nullptr);
        EXPECT_EQ(keyData5->keyCode, 32u);
        EXPECT_EQ(keyData5->keyName, "Space");
    }

    // Clean up
    if (std::filesystem::exists(exportFile))
    {
        std::filesystem::remove(exportFile);
    }
}
