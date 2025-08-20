// Copyright (c) 2025 JackLee
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <gtest/gtest.h>
#include <QApplication>
#include <spdlog/spdlog.h>

int main(int argc, char** argv)
{
    // Initialize Qt application for GUI tests
    QApplication app(argc, argv);

    // Set logging level to reduce noise during testing
    spdlog::set_level(spdlog::level::warn);

    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);

    // Run tests
    int result = RUN_ALL_TESTS();

    // Clean shutdown of logging to prevent issues
    spdlog::shutdown();

    return result;
}
