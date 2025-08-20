// Copyright (c) 2025 JackLee
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <gtest/gtest.h>
#include "core/Event.hpp"
#include "core/IEventStorage.hpp"
#include "storage/EventStorageFactory.hpp"
#include <filesystem>
#include <memory>
#include <fstream>

using namespace MouseRecorder::Core;
using namespace MouseRecorder::Storage;

class ExportFunctionalityTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create test events
        createTestEvents();

        // Clean up any test files from previous runs
        cleanupTestFiles();
    }

    void TearDown() override
    {
        cleanupTestFiles();
    }

    void createTestEvents()
    {
        m_testEvents.clear();

        // Create a variety of test events
        m_testEvents.push_back(EventFactory::createMouseMoveEvent({100, 200}));
        m_testEvents.push_back(
            EventFactory::createMouseClickEvent({150, 250}, MouseButton::Left));
        m_testEvents.push_back(EventFactory::createMouseDoubleClickEvent(
            {200, 300}, MouseButton::Right));
        // Use actual key codes with key names
        m_testEvents.push_back(
            EventFactory::createKeyPressEvent(65, "A", KeyModifier::Ctrl));
        m_testEvents.push_back(
            EventFactory::createKeyReleaseEvent(65, "A", KeyModifier::Ctrl));
    }

    void cleanupTestFiles()
    {
        std::vector<std::string> testFiles = {"test_export.json",
                                              "test_export.xml",
                                              "test_export.mre",
                                              "test_export_unique_json.json",
                                              "test_export_unique_xml.xml",
                                              "test_export_unique_binary.mre",
                                              "format_test.json",
                                              "format_test.xml",
                                              "format_test.mre"};

        for (const auto& file : testFiles)
        {
            if (std::filesystem::exists(file))
            {
                std::filesystem::remove(file);
            }
        }
    }

    void verifyFileFormat(const std::string& filename,
                          const std::string& expectedFormat)
    {
        EXPECT_TRUE(std::filesystem::exists(filename));

        std::ifstream file(filename);
        ASSERT_TRUE(file.is_open()) << "Could not open file: " << filename;

        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        file.close();

        if (expectedFormat == "json")
        {
            // JSON should contain these markers
            EXPECT_NE(content.find("{"), std::string::npos)
                << "JSON should contain opening brace";
            EXPECT_NE(content.find("\"metadata\""), std::string::npos)
                << "JSON should contain metadata field";
            EXPECT_NE(content.find("\"events\""), std::string::npos)
                << "JSON should contain events field";

            // Should NOT contain XML markers
            EXPECT_EQ(content.find("<?xml"), std::string::npos)
                << "JSON should not contain XML declaration";
            EXPECT_EQ(content.find("MouseRecorderEvents"), std::string::npos)
                << "JSON should not contain XML root element";
        }
        else if (expectedFormat == "xml")
        {
            // XML should contain these markers
            EXPECT_NE(content.find("<?xml"), std::string::npos)
                << "XML should contain XML declaration";
            EXPECT_NE(content.find("MouseRecorderEvents"), std::string::npos)
                << "XML should contain root element";
            EXPECT_NE(content.find("<Events"), std::string::npos)
                << "XML should contain Events element";

            // Should NOT contain JSON markers
            EXPECT_EQ(content.find("\"metadata\""), std::string::npos)
                << "XML should not contain JSON metadata field";
            EXPECT_EQ(content.find("\"events\""), std::string::npos)
                << "XML should not contain JSON events field";
        }
        else if (expectedFormat == "binary")
        {
            // Binary should not contain JSON or XML markers
            EXPECT_EQ(content.find("{"), std::string::npos)
                << "Binary should not contain JSON markers";
            EXPECT_EQ(content.find("<?xml"), std::string::npos)
                << "Binary should not contain XML markers";
            EXPECT_EQ(content.find("\"metadata\""), std::string::npos)
                << "Binary should not contain JSON metadata";
            EXPECT_EQ(content.find("MouseRecorderEvents"), std::string::npos)
                << "Binary should not contain XML root element";

            // Binary files should have specific binary header (magic number)
            EXPECT_GE(content.size(), 4)
                << "Binary file should have at least header";
        }
    }

    std::vector<std::unique_ptr<Event>> createEventCopies()
    {
        std::vector<std::unique_ptr<Event>> copies;
        for (const auto& event : m_testEvents)
        {
            copies.push_back(std::make_unique<Event>(*event));
        }
        return copies;
    }

    std::vector<std::unique_ptr<Event>> m_testEvents;
};

TEST_F(ExportFunctionalityTest, DirectStorageFormatExport)
{
    // Test direct export using storage classes to ensure they work correctly
    auto eventCopies = createEventCopies();

    // Test JSON export
    {
        auto storage = EventStorageFactory::createStorage(StorageFormat::Json);
        ASSERT_NE(storage, nullptr);

        std::string filename = "format_test.json";
        EXPECT_TRUE(storage->saveEvents(createEventCopies(), filename));
        verifyFileFormat(filename, "json");

        // Verify round-trip
        std::vector<std::unique_ptr<Event>> loadedEvents;
        StorageMetadata metadata;
        EXPECT_TRUE(storage->loadEvents(filename, loadedEvents, metadata));
        EXPECT_EQ(loadedEvents.size(), m_testEvents.size());
    }

    // Test XML export
    {
        auto storage = EventStorageFactory::createStorage(StorageFormat::Xml);
        ASSERT_NE(storage, nullptr);

        std::string filename = "format_test.xml";
        EXPECT_TRUE(storage->saveEvents(createEventCopies(), filename));
        verifyFileFormat(filename, "xml");

        // Verify round-trip
        std::vector<std::unique_ptr<Event>> loadedEvents;
        StorageMetadata metadata;
        EXPECT_TRUE(storage->loadEvents(filename, loadedEvents, metadata));
        EXPECT_EQ(loadedEvents.size(), m_testEvents.size());
    }

    // Test Binary export
    {
        auto storage =
            EventStorageFactory::createStorage(StorageFormat::Binary);
        ASSERT_NE(storage, nullptr);

        std::string filename = "format_test.mre";
        EXPECT_TRUE(storage->saveEvents(createEventCopies(), filename));
        verifyFileFormat(filename, "binary");

        // Verify round-trip
        std::vector<std::unique_ptr<Event>> loadedEvents;
        StorageMetadata metadata;
        EXPECT_TRUE(storage->loadEvents(filename, loadedEvents, metadata));
        EXPECT_EQ(loadedEvents.size(), m_testEvents.size());
    }
}

TEST_F(ExportFunctionalityTest, FileExtensionBasedFormatDetection)
{
    // Test that format is correctly detected from file extension

    // JSON extension
    {
        auto storage =
            EventStorageFactory::createStorageFromFilename("test.json");
        ASSERT_NE(storage, nullptr);
        EXPECT_EQ(storage->getSupportedFormat(), StorageFormat::Json);
        EXPECT_EQ(storage->getFileExtension(), ".json");
    }

    // XML extension
    {
        auto storage =
            EventStorageFactory::createStorageFromFilename("test.xml");
        ASSERT_NE(storage, nullptr);
        EXPECT_EQ(storage->getSupportedFormat(), StorageFormat::Xml);
        EXPECT_EQ(storage->getFileExtension(), ".xml");
    }

    // Binary extension
    {
        auto storage =
            EventStorageFactory::createStorageFromFilename("test.mre");
        ASSERT_NE(storage, nullptr);
        EXPECT_EQ(storage->getSupportedFormat(), StorageFormat::Binary);
        EXPECT_EQ(storage->getFileExtension(), ".mre");
    }
}

TEST_F(ExportFunctionalityTest, FilterBasedFormatSelection)
{
    // Test that format can be correctly determined from dialog filter strings

    std::vector<std::pair<std::string, StorageFormat>> filterTests = {
        {"JSON Event Recording (*.json)", StorageFormat::Json},
        {"XML Event Recording (*.xml)", StorageFormat::Xml},
        {"Binary Event Recording (*.mre)", StorageFormat::Binary}};

    for (const auto& [filterString, expectedFormat] : filterTests)
    {
        std::unique_ptr<IEventStorage> storage;

        // Simulate the filter-based selection logic from MainWindow
        if (filterString.find("JSON") != std::string::npos)
        {
            storage = EventStorageFactory::createStorage(StorageFormat::Json);
        }
        else if (filterString.find("XML") != std::string::npos)
        {
            storage = EventStorageFactory::createStorage(StorageFormat::Xml);
        }
        else if (filterString.find("Binary") != std::string::npos)
        {
            storage = EventStorageFactory::createStorage(StorageFormat::Binary);
        }

        ASSERT_NE(storage, nullptr);
        EXPECT_EQ(storage->getSupportedFormat(), expectedFormat)
            << "Filter: " << filterString;
    }
}

TEST_F(ExportFunctionalityTest, ExportWithoutExtensionUsesCorrectFormat)
{
    // Test that when a filename without extension is provided,
    // the format is determined from the selected filter

    auto eventCopies = createEventCopies();

    // Simulate exporting with filename "test_export" and different formats

    // Test JSON format
    {
        auto storage = EventStorageFactory::createStorage(StorageFormat::Json);
        ASSERT_NE(storage, nullptr);

        std::string filename = "test_export_unique_json";
        std::string finalFilename = filename + storage->getFileExtension();

        EXPECT_TRUE(storage->saveEvents(createEventCopies(), finalFilename));
        verifyFileFormat(finalFilename, "json");

        // Clean up immediately
        if (std::filesystem::exists(finalFilename))
        {
            std::filesystem::remove(finalFilename);
        }
    }

    // Test XML format
    {
        auto storage = EventStorageFactory::createStorage(StorageFormat::Xml);
        ASSERT_NE(storage, nullptr);

        std::string filename = "test_export_unique_xml";
        std::string finalFilename = filename + storage->getFileExtension();

        EXPECT_TRUE(storage->saveEvents(createEventCopies(), finalFilename));
        verifyFileFormat(finalFilename, "xml");

        // Clean up immediately
        if (std::filesystem::exists(finalFilename))
        {
            std::filesystem::remove(finalFilename);
        }
    }

    // Test Binary format
    {
        auto storage =
            EventStorageFactory::createStorage(StorageFormat::Binary);
        ASSERT_NE(storage, nullptr);

        std::string filename = "test_export_unique_binary";
        std::string finalFilename = filename + storage->getFileExtension();

        EXPECT_TRUE(storage->saveEvents(createEventCopies(), finalFilename));
        verifyFileFormat(finalFilename, "binary");

        // Clean up immediately
        if (std::filesystem::exists(finalFilename))
        {
            std::filesystem::remove(finalFilename);
        }
    }
}

TEST_F(ExportFunctionalityTest, FileDialogFilterGeneration)
{
    // Test that the file dialog filter is correctly generated
    std::string filter = EventStorageFactory::getFileDialogFilter();

    // Should contain all supported formats
    EXPECT_NE(filter.find("JSON Event Recording"), std::string::npos);
    EXPECT_NE(filter.find("XML Event Recording"), std::string::npos);
    EXPECT_NE(filter.find("Binary Event Recording"), std::string::npos);

    // Should contain file extensions
    EXPECT_NE(filter.find("*.json"), std::string::npos);
    EXPECT_NE(filter.find("*.xml"), std::string::npos);
    EXPECT_NE(filter.find("*.mre"), std::string::npos);

    // Should contain "All supported files" option
    EXPECT_NE(filter.find("All supported files"), std::string::npos);
}

TEST_F(ExportFunctionalityTest, ErrorHandlingForInvalidFormat)
{
    // Test error handling when an invalid format is requested

    // Test with unsupported extension
    auto storage =
        EventStorageFactory::createStorageFromFilename("test.unsupported");

    // Should fallback to JSON
    ASSERT_NE(storage, nullptr);
    EXPECT_EQ(storage->getSupportedFormat(), StorageFormat::Json);
}

TEST_F(ExportFunctionalityTest, EmptyEventsExport)
{
    // Test exporting empty event list
    std::vector<std::unique_ptr<Event>> emptyEvents;

    auto storage = EventStorageFactory::createStorage(StorageFormat::Json);
    ASSERT_NE(storage, nullptr);

    std::string filename = "empty_export.json";
    EXPECT_TRUE(storage->saveEvents(emptyEvents, filename));
    EXPECT_TRUE(std::filesystem::exists(filename));

    // Load and verify
    std::vector<std::unique_ptr<Event>> loadedEvents;
    StorageMetadata metadata;
    EXPECT_TRUE(storage->loadEvents(filename, loadedEvents, metadata));
    EXPECT_TRUE(loadedEvents.empty());

    // Clean up
    std::filesystem::remove(filename);
}
