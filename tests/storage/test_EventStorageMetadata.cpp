#include <gtest/gtest.h>
#include "storage/JsonEventStorage.hpp"
#include "storage/XmlEventStorage.hpp"
#include "storage/BinaryEventStorage.hpp"
#include "storage/EventStorageFactory.hpp"
#include "core/Event.hpp"
#include <filesystem>
#include <fstream>
#include <QCoreApplication>

using namespace MouseRecorder::Storage;
using namespace MouseRecorder::Core;

class EventStorageMetadataTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create test events
        m_testEvents.push_back(EventFactory::createMouseMoveEvent({100, 200}));
        m_testEvents.push_back(
          EventFactory::createMouseClickEvent({150, 250}, MouseButton::Left)
        );
        m_testEvents.push_back(
          EventFactory::createKeyPressEvent(65, "A", KeyModifier::Ctrl)
        );

        // Create test metadata
        m_testMetadata.version = "1.0.0";
        m_testMetadata.applicationName = "MouseRecorder";
        m_testMetadata.createdBy = "TestUser";
        m_testMetadata.description = "Test recording for metadata validation";
        m_testMetadata.creationTimestamp =
          1692201600000; // Fixed timestamp for testing
        m_testMetadata.totalDurationMs = 5000;
        m_testMetadata.totalEvents = m_testEvents.size();
        m_testMetadata.platform = "Linux Test Platform";
        m_testMetadata.screenResolution = "1920x1080";
    }

    void TearDown() override
    {
        // Clean up test files
        cleanupTestFiles();
    }

    void cleanupTestFiles()
    {
        std::vector<std::string> testFiles = {
          "test_metadata.json", "test_metadata.xml", "test_metadata.mre"
        };

        for (const auto& file : testFiles)
        {
            if (std::filesystem::exists(file))
            {
                std::filesystem::remove(file);
            }
        }
    }

    void verifyMetadata(
      const StorageMetadata& original, const StorageMetadata& loaded
    )
    {
        EXPECT_EQ(original.version, loaded.version);
        EXPECT_EQ(original.applicationName, loaded.applicationName);
        EXPECT_EQ(original.createdBy, loaded.createdBy);
        EXPECT_EQ(original.description, loaded.description);
        EXPECT_EQ(original.creationTimestamp, loaded.creationTimestamp);
        EXPECT_EQ(original.totalDurationMs, loaded.totalDurationMs);
        EXPECT_EQ(original.totalEvents, loaded.totalEvents);
        EXPECT_EQ(original.platform, loaded.platform);
        EXPECT_EQ(original.screenResolution, loaded.screenResolution);
    }

    std::vector<std::unique_ptr<Event>> m_testEvents;
    StorageMetadata m_testMetadata;
};

TEST_F(EventStorageMetadataTest, JsonMetadataSaveAndLoad)
{
    JsonEventStorage storage;
    const std::string filename = "test_metadata.json";

    // Save events with metadata
    ASSERT_TRUE(storage.saveEvents(m_testEvents, filename, m_testMetadata));
    ASSERT_TRUE(std::filesystem::exists(filename));

    // Load events and metadata
    std::vector<std::unique_ptr<Event>> loadedEvents;
    StorageMetadata loadedMetadata;

    ASSERT_TRUE(storage.loadEvents(filename, loadedEvents, loadedMetadata));

    // Verify metadata was preserved
    verifyMetadata(m_testMetadata, loadedMetadata);

    // Verify events were preserved
    EXPECT_EQ(loadedEvents.size(), m_testEvents.size());
}

TEST_F(EventStorageMetadataTest, XmlMetadataSaveAndLoad)
{
    XmlEventStorage storage;
    const std::string filename = "test_metadata.xml";

    // Save events with metadata
    ASSERT_TRUE(storage.saveEvents(m_testEvents, filename, m_testMetadata));
    ASSERT_TRUE(std::filesystem::exists(filename));

    // Load events and metadata
    std::vector<std::unique_ptr<Event>> loadedEvents;
    StorageMetadata loadedMetadata;

    ASSERT_TRUE(storage.loadEvents(filename, loadedEvents, loadedMetadata));

    // Verify metadata was preserved
    verifyMetadata(m_testMetadata, loadedMetadata);

    // Verify events were preserved
    EXPECT_EQ(loadedEvents.size(), m_testEvents.size());
}

TEST_F(EventStorageMetadataTest, BinaryMetadataSaveAndLoad)
{
    BinaryEventStorage storage;
    const std::string filename = "test_metadata.mre";

    // Save events with metadata
    ASSERT_TRUE(storage.saveEvents(m_testEvents, filename, m_testMetadata));
    ASSERT_TRUE(std::filesystem::exists(filename));

    // Load events and metadata
    std::vector<std::unique_ptr<Event>> loadedEvents;
    StorageMetadata loadedMetadata;

    ASSERT_TRUE(storage.loadEvents(filename, loadedEvents, loadedMetadata));

    // Verify metadata was preserved
    verifyMetadata(m_testMetadata, loadedMetadata);

    // Verify events were preserved
    EXPECT_EQ(loadedEvents.size(), m_testEvents.size());
}

TEST_F(EventStorageMetadataTest, EmptyMetadataHandling)
{
    JsonEventStorage storage;
    const std::string filename = "test_metadata_empty.json";

    // Save events with default (empty) metadata
    StorageMetadata emptyMetadata; // Uses default values
    ASSERT_TRUE(storage.saveEvents(m_testEvents, filename, emptyMetadata));

    // Load and verify default values were properly serialized
    std::vector<std::unique_ptr<Event>> loadedEvents;
    StorageMetadata loadedMetadata;

    ASSERT_TRUE(storage.loadEvents(filename, loadedEvents, loadedMetadata));

    // Verify default values
    EXPECT_EQ(loadedMetadata.version, "1.0.0");
    EXPECT_EQ(loadedMetadata.applicationName, "MouseRecorder");
    EXPECT_EQ(loadedMetadata.createdBy, "");
    EXPECT_EQ(loadedMetadata.description, "");
    EXPECT_EQ(loadedMetadata.creationTimestamp, 0UL);
    EXPECT_EQ(loadedMetadata.totalDurationMs, 0UL);
    EXPECT_EQ(loadedMetadata.totalEvents, 0UL);
    EXPECT_EQ(loadedMetadata.platform, "");
    EXPECT_EQ(loadedMetadata.screenResolution, "");

    // Clean up
    if (std::filesystem::exists(filename))
    {
        std::filesystem::remove(filename);
    }
}

TEST_F(EventStorageMetadataTest, XmlMetadataFieldVerification)
{
    XmlEventStorage storage;
    const std::string filename = "test_metadata_verification.xml";

    // Save events with metadata
    ASSERT_TRUE(storage.saveEvents(m_testEvents, filename, m_testMetadata));

    // Read the raw XML file to verify field names and values
    std::ifstream file(filename);
    ASSERT_TRUE(file.is_open());

    std::string content(
      (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()
    );
    file.close();

    // Verify XML contains all metadata fields with correct values
    EXPECT_NE(content.find("<Version>1.0.0</Version>"), std::string::npos);
    EXPECT_NE(
      content.find("<ApplicationName>MouseRecorder</ApplicationName>"),
      std::string::npos
    );
    EXPECT_NE(
      content.find("<CreatedBy>TestUser</CreatedBy>"), std::string::npos
    );
    EXPECT_NE(
      content.find(
        "<Description>Test recording for metadata validation</Description>"
      ),
      std::string::npos
    );
    EXPECT_NE(
      content.find("<CreationTimestamp>1692201600000</CreationTimestamp>"),
      std::string::npos
    );
    EXPECT_NE(
      content.find("<TotalDurationMs>5000</TotalDurationMs>"), std::string::npos
    );
    EXPECT_NE(content.find("<TotalEvents>3</TotalEvents>"), std::string::npos);
    EXPECT_NE(
      content.find("<Platform>Linux Test Platform</Platform>"),
      std::string::npos
    );
    EXPECT_NE(
      content.find("<ScreenResolution>1920x1080</ScreenResolution>"),
      std::string::npos
    );

    // Clean up
    if (std::filesystem::exists(filename))
    {
        std::filesystem::remove(filename);
    }
}

TEST_F(EventStorageMetadataTest, MetadataWithSpecialCharacters)
{
    XmlEventStorage storage;
    const std::string filename = "test_metadata_special.xml";

    // Create metadata with special characters that need XML escaping
    StorageMetadata specialMetadata;
    specialMetadata.version = "1.0.0";
    specialMetadata.applicationName = "MouseRecorder";
    specialMetadata.createdBy = "User<>&\"'Test";
    specialMetadata.description = "Test with special chars: <>&\"'";
    specialMetadata.platform = "Linux & Unix";

    // Save and load
    ASSERT_TRUE(storage.saveEvents(m_testEvents, filename, specialMetadata));

    std::vector<std::unique_ptr<Event>> loadedEvents;
    StorageMetadata loadedMetadata;
    ASSERT_TRUE(storage.loadEvents(filename, loadedEvents, loadedMetadata));

    // Verify special characters were properly handled
    EXPECT_EQ(loadedMetadata.createdBy, "User<>&\"'Test");
    EXPECT_EQ(loadedMetadata.description, "Test with special chars: <>&\"'");
    EXPECT_EQ(loadedMetadata.platform, "Linux & Unix");

    // Clean up
    if (std::filesystem::exists(filename))
    {
        std::filesystem::remove(filename);
    }
}
