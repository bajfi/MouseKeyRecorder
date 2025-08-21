// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

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

// Additional tests for file format detection and extension handling
class EventStorageFormatTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create a simple test event
        m_testEvents.push_back(EventFactory::createMouseMoveEvent({100, 200}));
        m_testEvents.push_back(
            EventFactory::createMouseClickEvent({150, 250}, MouseButton::Left));
    }

    void TearDown() override
    {
        // Clean up any test files
        std::vector<std::string> testFiles = {"test.json",
                                              "test.xml",
                                              "test.mre",
                                              "test.bin",
                                              "test_file.json",
                                              "test_file.xml",
                                              "test_file.mre",
                                              "export_test.json",
                                              "export_test.xml",
                                              "export_test.mre"};

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
            copies.push_back(std::make_unique<Event>(*event));
        }
        return copies;
    }

    std::vector<std::unique_ptr<Event>> m_testEvents;
};

TEST_F(EventStorageFormatTest, FileExtensionDetection)
{
    // Test JSON detection
    auto jsonStorage =
        EventStorageFactory::createStorageFromFilename("test.json");
    ASSERT_NE(jsonStorage, nullptr);
    EXPECT_EQ(jsonStorage->getFileExtension(), ".json");
    EXPECT_EQ(jsonStorage->getFormatDescription(), "JSON Event Recording");

    // Test XML detection
    auto xmlStorage =
        EventStorageFactory::createStorageFromFilename("test.xml");
    ASSERT_NE(xmlStorage, nullptr);
    EXPECT_EQ(xmlStorage->getFileExtension(), ".xml");
    EXPECT_EQ(xmlStorage->getFormatDescription(), "XML Event Recording");

    // Test Binary detection (.mre extension)
    auto binaryStorage =
        EventStorageFactory::createStorageFromFilename("test.mre");
    ASSERT_NE(binaryStorage, nullptr);
    EXPECT_EQ(binaryStorage->getFileExtension(), ".mre");
    EXPECT_EQ(binaryStorage->getFormatDescription(), "Binary Event Recording");
}

TEST_F(EventStorageFormatTest, IncorrectExtensionFallback)
{
    // Test that .bin extension doesn't work for binary (should fallback to
    // JSON)
    auto storage = EventStorageFactory::createStorageFromFilename("test.bin");
    ASSERT_NE(storage, nullptr);
    // Should fallback to JSON format
    EXPECT_EQ(storage->getFileExtension(), ".json");
    EXPECT_EQ(storage->getFormatDescription(), "JSON Event Recording");
}

TEST_F(EventStorageFormatTest, UnknownExtensionFallback)
{
    // Test unknown extension defaults to JSON
    auto storage =
        EventStorageFactory::createStorageFromFilename("test.unknown");
    ASSERT_NE(storage, nullptr);
    EXPECT_EQ(storage->getFileExtension(), ".json");
    EXPECT_EQ(storage->getFormatDescription(), "JSON Event Recording");
}

TEST_F(EventStorageFormatTest, CaseInsensitiveExtensions)
{
    // Test case insensitive extension detection
    auto jsonStorage =
        EventStorageFactory::createStorageFromFilename("test.JSON");
    ASSERT_NE(jsonStorage, nullptr);
    EXPECT_EQ(jsonStorage->getFileExtension(), ".json");

    auto xmlStorage =
        EventStorageFactory::createStorageFromFilename("test.XML");
    ASSERT_NE(xmlStorage, nullptr);
    EXPECT_EQ(xmlStorage->getFileExtension(), ".xml");

    auto binaryStorage =
        EventStorageFactory::createStorageFromFilename("test.MRE");
    ASSERT_NE(binaryStorage, nullptr);
    EXPECT_EQ(binaryStorage->getFileExtension(), ".mre");
}

TEST_F(EventStorageFormatTest, FileDialogFilterGeneration)
{
    std::string filter = EventStorageFactory::getFileDialogFilter();

    // Should contain all supported formats
    EXPECT_NE(filter.find("*.json"), std::string::npos);
    EXPECT_NE(filter.find("*.xml"), std::string::npos);
    EXPECT_NE(filter.find("*.mre"), std::string::npos);

    // Should NOT contain .bin extension
    EXPECT_EQ(filter.find("*.bin"), std::string::npos);

    // Should contain format descriptions
    EXPECT_NE(filter.find("JSON Event Recording"), std::string::npos);
    EXPECT_NE(filter.find("XML Event Recording"), std::string::npos);
    EXPECT_NE(filter.find("Binary Event Recording"), std::string::npos);
}

TEST_F(EventStorageFormatTest, ActualFormatVerification)
{
    // This test verifies that each storage format actually saves in the
    // expected format
    auto eventCopies = createEventCopies(m_testEvents);

    // Test JSON format
    {
        auto storage = EventStorageFactory::createStorage(StorageFormat::Json);
        ASSERT_NE(storage, nullptr);

        std::string filename = "export_test.json";
        EXPECT_TRUE(
            storage->saveEvents(createEventCopies(m_testEvents), filename));
        EXPECT_TRUE(std::filesystem::exists(filename));

        // Verify it's actually JSON by checking file content
        std::ifstream file(filename);
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        file.close();

        // JSON should contain these strings
        EXPECT_NE(content.find("{"), std::string::npos);
        EXPECT_NE(content.find("\"metadata\""), std::string::npos);
        EXPECT_NE(content.find("\"events\""), std::string::npos);
    }

    // Test XML format
    {
        auto storage = EventStorageFactory::createStorage(StorageFormat::Xml);
        ASSERT_NE(storage, nullptr);

        std::string filename = "export_test.xml";
        bool saveResult =
            storage->saveEvents(createEventCopies(m_testEvents), filename);
        if (!saveResult)
        {
            ADD_FAILURE() << "Failed to save XML events: "
                          << storage->getLastError();
        }
        EXPECT_TRUE(saveResult);
        EXPECT_TRUE(std::filesystem::exists(filename));

        // Verify it's actually XML by checking file content
        std::ifstream file(filename);
        if (!file.is_open())
        {
            ADD_FAILURE() << "Failed to open XML file for reading";
        }
        else
        {
            std::string content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
            file.close();

            // Debug: print content if test fails
            if (content.find("<?xml") == std::string::npos)
            {
                ADD_FAILURE()
                    << "XML content doesn't contain XML declaration. Content: "
                    << content;
            }
            if (content.find("MouseRecorderEvents") == std::string::npos)
            {
                ADD_FAILURE() << "XML content doesn't contain "
                                 "MouseRecorderEvents tag. Content: "
                              << content;
            }

            // XML should contain these strings
            EXPECT_NE(content.find("<?xml"), std::string::npos);
            EXPECT_NE(content.find("MouseRecorderEvents"), std::string::npos);
            EXPECT_NE(content.find("<Events"), std::string::npos);
        }
    }

    // Test Binary format
    {
        auto storage =
            EventStorageFactory::createStorage(StorageFormat::Binary);
        ASSERT_NE(storage, nullptr);

        std::string filename = "export_test.mre";
        EXPECT_TRUE(
            storage->saveEvents(createEventCopies(m_testEvents), filename));
        EXPECT_TRUE(std::filesystem::exists(filename));

        // Verify it's actually binary (won't contain readable JSON/XML markers)
        std::ifstream file(filename);
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        file.close();

        // Binary should NOT contain JSON or XML markers
        EXPECT_EQ(content.find("\"metadata\""), std::string::npos);
        EXPECT_EQ(content.find("<?xml"), std::string::npos);
        EXPECT_EQ(content.find("MouseRecorderEvents"), std::string::npos);
    }
}

TEST_F(EventStorageFormatTest, RoundTripConsistency)
{
    // Test that each format can save and load the same data correctly

    std::vector<StorageFormat> formats = {
        StorageFormat::Json, StorageFormat::Xml, StorageFormat::Binary};

    std::vector<std::string> filenames = {
        "test_file.json", "test_file.xml", "test_file.mre"};

    for (size_t i = 0; i < formats.size(); ++i)
    {
        auto storage = EventStorageFactory::createStorage(formats[i]);
        ASSERT_NE(storage, nullptr) << "Failed to create storage for format "
                                    << static_cast<int>(formats[i]);

        // Save events
        auto eventCopies = createEventCopies(m_testEvents);
        EXPECT_TRUE(storage->saveEvents(eventCopies, filenames[i]))
            << "Failed to save events in format "
            << static_cast<int>(formats[i]);
        EXPECT_TRUE(std::filesystem::exists(filenames[i]));

        // Load events back
        std::vector<std::unique_ptr<Event>> loadedEvents;
        StorageMetadata metadata;
        EXPECT_TRUE(storage->loadEvents(filenames[i], loadedEvents, metadata))
            << "Failed to load events from format "
            << static_cast<int>(formats[i]);

        // Verify the data matches
        EXPECT_EQ(loadedEvents.size(), m_testEvents.size())
            << "Event count mismatch in format "
            << static_cast<int>(formats[i]);

        if (loadedEvents.size() == m_testEvents.size())
        {
            for (size_t j = 0; j < loadedEvents.size(); ++j)
            {
                EXPECT_EQ(loadedEvents[j]->getType(),
                          m_testEvents[j]->getType())
                    << "Event type mismatch at index " << j << " in format "
                    << static_cast<int>(formats[i]);
            }
        }
    }
}
