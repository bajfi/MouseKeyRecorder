#include <gtest/gtest.h>
#include "application/MouseRecorderApp.hpp"
#include "core/Event.hpp"
#include "core/MouseMovementOptimizer.hpp"
#include "storage/EventStorageFactory.hpp"
#include <filesystem>
#include <fstream>

using namespace MouseRecorder::Application;
using namespace MouseRecorder::Core;
using namespace MouseRecorder::Storage;

class MouseMovementOptimizationIntegrationTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        m_app = std::make_unique<MouseRecorderApp>();
        ASSERT_TRUE(m_app->initialize());

        // Set up temporary test directory
        m_testDir = std::filesystem::temp_directory_path() /
                    "mouserecorder_optimization_test";
        std::filesystem::create_directories(m_testDir);
    }

    void TearDown() override
    {
        if (m_app)
        {
            m_app->shutdown();
            m_app.reset();
        }

        // Clean up test files
        if (std::filesystem::exists(m_testDir))
        {
            std::filesystem::remove_all(m_testDir);
        }
    }

    // Helper to create test events with redundant mouse movements
    std::vector<std::unique_ptr<Event>> createTestEventSequence()
    {
        std::vector<std::unique_ptr<Event>> events;
        auto baseTime = std::chrono::steady_clock::now();

        // Create a sequence with many redundant movements
        for (int i = 0; i < 100; ++i)
        {
            MouseEventData mouseData;
            mouseData.position =
              Point(i, i / 10); // Mostly horizontal with some vertical

            auto timestamp =
              baseTime + std::chrono::milliseconds(i * 10); // 10ms intervals
            events.push_back(
              std::make_unique<Event>(
                EventType::MouseMove, mouseData, timestamp
              )
            );
        }

        // Add a click in the middle
        MouseEventData clickData;
        clickData.position = Point(50, 5);
        clickData.button = MouseButton::Left;
        auto clickTime = baseTime + std::chrono::milliseconds(50 * 10);
        events.push_back(
          std::make_unique<Event>(EventType::MouseClick, clickData, clickTime)
        );

        return events;
    }

    std::unique_ptr<MouseRecorderApp> m_app;
    std::filesystem::path m_testDir;
};

TEST_F(MouseMovementOptimizationIntegrationTest, ConfigurationIntegration)
{
    auto& config = m_app->getConfiguration();

    // Test default configuration values (check if they exist, don't assume
    // specific values since they might be persisted from previous tests)
    int thresholdValue = config.getInt(ConfigKeys::MOUSE_MOVEMENT_THRESHOLD, 0);

    // The settings should exist (not use default values)
    EXPECT_TRUE(
      config.getBool(ConfigKeys::OPTIMIZE_MOUSE_MOVEMENTS, false) ||
      !config.getBool(ConfigKeys::OPTIMIZE_MOUSE_MOVEMENTS, true)
    );                            // Either true or false, not default
    EXPECT_GT(thresholdValue, 0); // Should have a positive threshold value

    // Test changing configuration values
    config.setBool(ConfigKeys::OPTIMIZE_MOUSE_MOVEMENTS, false);
    EXPECT_FALSE(config.getBool(ConfigKeys::OPTIMIZE_MOUSE_MOVEMENTS, true));

    config.setInt(ConfigKeys::MOUSE_MOVEMENT_THRESHOLD, 15);
    EXPECT_EQ(config.getInt(ConfigKeys::MOUSE_MOVEMENT_THRESHOLD, 0), 15);

    // Restore a reasonable value for other tests
    config.setBool(ConfigKeys::OPTIMIZE_MOUSE_MOVEMENTS, true);
    config.setInt(ConfigKeys::MOUSE_MOVEMENT_THRESHOLD, 5);
}

TEST_F(MouseMovementOptimizationIntegrationTest, OptimizationAppliedDuringSave)
{
    // Create test events
    auto events = createTestEventSequence();
    size_t originalEventCount = events.size();

    // Enable optimization
    auto& config = m_app->getConfiguration();
    config.setBool(ConfigKeys::OPTIMIZE_MOUSE_MOVEMENTS, true);
    config.setInt(ConfigKeys::MOUSE_MOVEMENT_THRESHOLD, 5);

    // Apply optimization directly (simulating what happens in MainWindow)
    MouseMovementOptimizer::OptimizationConfig optimizationConfig;
    optimizationConfig.enabled = true;
    optimizationConfig.strategy =
      MouseMovementOptimizer::OptimizationStrategy::Combined;
    optimizationConfig.distanceThreshold =
      config.getInt(ConfigKeys::MOUSE_MOVEMENT_THRESHOLD, 5);
    optimizationConfig.timeThresholdMs = 16;
    optimizationConfig.douglasPeuckerEpsilon = 2.0;
    optimizationConfig.preserveClicks = true;
    optimizationConfig.preserveFirstLast = true;

    size_t removedCount =
      MouseMovementOptimizer::optimizeEvents(events, optimizationConfig);

    EXPECT_GT(removedCount, 0);
    EXPECT_LT(events.size(), originalEventCount);

    // Verify the click is preserved
    bool hasClick = false;
    for (const auto& event : events)
    {
        if (event->getType() == EventType::MouseClick)
        {
            hasClick = true;
            break;
        }
    }
    EXPECT_TRUE(hasClick);
}

TEST_F(MouseMovementOptimizationIntegrationTest, OptimizationDisabled)
{
    // Create test events
    auto events = createTestEventSequence();
    size_t originalEventCount = events.size();

    // Disable optimization
    auto& config = m_app->getConfiguration();
    config.setBool(ConfigKeys::OPTIMIZE_MOUSE_MOVEMENTS, false);

    // Apply optimization configuration (should not optimize when disabled)
    MouseMovementOptimizer::OptimizationConfig optimizationConfig;
    optimizationConfig.enabled =
      config.getBool(ConfigKeys::OPTIMIZE_MOUSE_MOVEMENTS, true);

    size_t removedCount =
      MouseMovementOptimizer::optimizeEvents(events, optimizationConfig);

    EXPECT_EQ(removedCount, 0);
    EXPECT_EQ(events.size(), originalEventCount);
}

TEST_F(MouseMovementOptimizationIntegrationTest, SaveAndLoadWithOptimization)
{
    // Create test events
    auto originalEvents = createTestEventSequence();
    size_t originalEventCount = originalEvents.size();

    // Enable optimization
    auto& config = m_app->getConfiguration();
    config.setBool(ConfigKeys::OPTIMIZE_MOUSE_MOVEMENTS, true);
    config.setInt(ConfigKeys::MOUSE_MOVEMENT_THRESHOLD, 10);

    // Create a copy for optimization (simulating save process)
    std::vector<std::unique_ptr<Event>> eventsToSave;
    for (const auto& event : originalEvents)
    {
        eventsToSave.emplace_back(std::make_unique<Event>(*event));
    }

    // Apply optimization
    MouseMovementOptimizer::OptimizationConfig optimizationConfig;
    optimizationConfig.enabled = true;
    optimizationConfig.strategy =
      MouseMovementOptimizer::OptimizationStrategy::Combined;
    optimizationConfig.distanceThreshold =
      config.getInt(ConfigKeys::MOUSE_MOVEMENT_THRESHOLD, 5);
    optimizationConfig.timeThresholdMs = 16;
    optimizationConfig.douglasPeuckerEpsilon = 2.0;
    optimizationConfig.preserveClicks = true;
    optimizationConfig.preserveFirstLast = true;

    MouseMovementOptimizer::optimizeEvents(eventsToSave, optimizationConfig);

    EXPECT_LT(eventsToSave.size(), originalEventCount);

    // Save optimized events
    std::string testFile = (m_testDir / "optimized_test.json").string();
    auto storage = EventStorageFactory::createStorageFromFilename(testFile);
    ASSERT_NE(storage, nullptr);

    StorageMetadata metadata;
    metadata.version = "0.0.1";
    metadata.totalEvents = eventsToSave.size();

    EXPECT_TRUE(storage->saveEvents(eventsToSave, testFile, metadata));

    // Load events back
    std::vector<std::unique_ptr<Event>> loadedEvents;
    StorageMetadata loadedMetadata;

    EXPECT_TRUE(storage->loadEvents(testFile, loadedEvents, loadedMetadata));

    // Verify the loaded events match the optimized count
    EXPECT_EQ(loadedEvents.size(), eventsToSave.size());
    EXPECT_LT(loadedEvents.size(), originalEventCount);

    // Verify metadata
    EXPECT_EQ(loadedMetadata.totalEvents, eventsToSave.size());
}

TEST_F(MouseMovementOptimizationIntegrationTest, DifferentThresholdValues)
{
    auto events1 = createTestEventSequence();
    auto events2 = createTestEventSequence();

    // Test with different threshold values
    MouseMovementOptimizer::OptimizationConfig config1;
    config1.enabled = true;
    config1.strategy =
      MouseMovementOptimizer::OptimizationStrategy::DistanceThreshold;
    config1.distanceThreshold = 5;

    MouseMovementOptimizer::OptimizationConfig config2;
    config2.enabled = true;
    config2.strategy =
      MouseMovementOptimizer::OptimizationStrategy::DistanceThreshold;
    config2.distanceThreshold = 20;

    size_t removed1 = MouseMovementOptimizer::optimizeEvents(events1, config1);
    size_t removed2 = MouseMovementOptimizer::optimizeEvents(events2, config2);

    // Higher threshold should remove more events
    EXPECT_GT(removed2, removed1);
    EXPECT_LT(events2.size(), events1.size());
}

TEST_F(MouseMovementOptimizationIntegrationTest, PreserveImportantEvents)
{
    std::vector<std::unique_ptr<Event>> events;
    auto baseTime = std::chrono::steady_clock::now();

    // Create sequence: move -> move -> click -> move -> doubleclick -> move
    MouseEventData moveData1;
    moveData1.position = Point(0, 0);
    events.push_back(
      std::make_unique<Event>(EventType::MouseMove, moveData1, baseTime)
    );

    MouseEventData moveData2;
    moveData2.position = Point(1, 0);
    events.push_back(
      std::make_unique<Event>(
        EventType::MouseMove,
        moveData2,
        baseTime + std::chrono::milliseconds(10)
      )
    );

    MouseEventData clickData;
    clickData.position = Point(2, 0);
    clickData.button = MouseButton::Left;
    events.push_back(
      std::make_unique<Event>(
        EventType::MouseClick,
        clickData,
        baseTime + std::chrono::milliseconds(20)
      )
    );

    MouseEventData moveData3;
    moveData3.position = Point(3, 0);
    events.push_back(
      std::make_unique<Event>(
        EventType::MouseMove,
        moveData3,
        baseTime + std::chrono::milliseconds(30)
      )
    );

    MouseEventData doubleClickData;
    doubleClickData.position = Point(4, 0);
    doubleClickData.button = MouseButton::Left;
    events.push_back(
      std::make_unique<Event>(
        EventType::MouseDoubleClick,
        doubleClickData,
        baseTime + std::chrono::milliseconds(40)
      )
    );

    MouseEventData moveData4;
    moveData4.position = Point(5, 0);
    events.push_back(
      std::make_unique<Event>(
        EventType::MouseMove,
        moveData4,
        baseTime + std::chrono::milliseconds(50)
      )
    );

    MouseMovementOptimizer::OptimizationConfig config;
    config.enabled = true;
    config.strategy =
      MouseMovementOptimizer::OptimizationStrategy::DistanceThreshold;
    config.distanceThreshold = 2;
    config.preserveClicks = true;

    MouseMovementOptimizer::optimizeEvents(events, config);

    // Verify clicks are preserved
    int clickCount = 0;
    int doubleClickCount = 0;
    for (const auto& event : events)
    {
        if (event->getType() == EventType::MouseClick)
            clickCount++;
        else if (event->getType() == EventType::MouseDoubleClick)
            doubleClickCount++;
    }

    EXPECT_EQ(clickCount, 1);
    EXPECT_EQ(doubleClickCount, 1);

    // Should have preserved some movement events adjacent to clicks
    EXPECT_GE(events.size(), 3); // At least the two clicks + some moves
}

// Test to verify the optimization works with different file formats
TEST_F(
  MouseMovementOptimizationIntegrationTest, OptimizationWithDifferentFormats
)
{
    auto events = createTestEventSequence();
    size_t originalCount = events.size();

    // Apply optimization
    MouseMovementOptimizer::OptimizationConfig config;
    config.enabled = true;
    config.strategy = MouseMovementOptimizer::OptimizationStrategy::Combined;
    config.distanceThreshold = 10;
    MouseMovementOptimizer::optimizeEvents(events, config);

    size_t optimizedCount = events.size();
    EXPECT_LT(optimizedCount, originalCount);

    // Test saving with different formats
    std::vector<std::string> formats = {".json", ".xml", ".mre"};

    for (const auto& format : formats)
    {
        std::string testFile =
          (m_testDir / ("test_optimized" + format)).string();
        auto storage = EventStorageFactory::createStorageFromFilename(testFile);
        ASSERT_NE(storage, nullptr);

        StorageMetadata metadata;
        metadata.version = "0.0.1";
        metadata.totalEvents = events.size();

        // Save optimized events
        EXPECT_TRUE(storage->saveEvents(events, testFile, metadata));

        // Verify file exists and is not empty
        EXPECT_TRUE(std::filesystem::exists(testFile));
        EXPECT_GT(std::filesystem::file_size(testFile), 0);

        // Load and verify
        std::vector<std::unique_ptr<Event>> loadedEvents;
        StorageMetadata loadedMetadata;
        EXPECT_TRUE(
          storage->loadEvents(testFile, loadedEvents, loadedMetadata)
        );
        EXPECT_EQ(loadedEvents.size(), optimizedCount);
    }
}
