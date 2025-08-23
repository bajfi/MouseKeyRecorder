cmake_minimum_required(VERSION 3.20)

project(MouseRecorder
    VERSION 0.0.1
    DESCRIPTION "Cross-platform Mouse and Keyboard Event Recorder"
    LANGUAGES CXX
)

# Generate version information
include(cmake/version.cmake)

# Options
option(BUILD_TESTS "Build tests" OFF)
option(USE_SYSTEM_DEPS "Use system dependencies instead of fetching" ON)

# Serialization options
option(USE_QT_SERIALIZATION "Use Qt-based serialization (always available)" ON)
option(USE_NLOHMANN_JSON "Use nlohmann::json for JSON serialization" OFF)
option(USE_PUGIXML "Use pugixml for XML serialization" OFF)
option(DEFAULT_QT_SERIALIZATION "Use Qt serialization by default when both are available" ON)

# Set C++23 standard globally
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable modern C++ features and warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include FetchContent for dependency management
include(FetchContent)

# Find Qt (try Qt6 first, fallback to Qt5)
find_package(Qt6 QUIET COMPONENTS Core Widgets Test Gui Xml)
if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
    message(STATUS "Using Qt6")
else()
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets Test Gui Xml)
    set(QT_VERSION_MAJOR 5)
    message(STATUS "Using Qt5")
endif()

# Enable Qt MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Set UIC search paths
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/src/gui/ui)

# Function to find or fetch dependencies
function(find_or_fetch_dependency name git_url git_tag find_package_name)
    if(USE_SYSTEM_DEPS)
        find_package(${find_package_name} QUIET)
        if(NOT ${find_package_name}_FOUND)
            find_package(PkgConfig QUIET)
            if(PkgConfig_FOUND)
                pkg_check_modules(${name} ${name})
            endif()
        endif()
    endif()

    if(NOT ${find_package_name}_FOUND AND NOT ${name}_FOUND)
        message(STATUS "Fetching ${name} from ${git_url}")
        FetchContent_Declare(
            ${name}
            GIT_REPOSITORY ${git_url}
            GIT_TAG ${git_tag}
        )
        FetchContent_MakeAvailable(${name})
    endif()
endfunction()

# Find or fetch third-party dependencies based on serialization options
if(USE_NLOHMANN_JSON)
    find_or_fetch_dependency(
        nlohmann_json
        "https://github.com/nlohmann/json.git"
        "v3.11.3"
        "nlohmann_json"
    )
endif()

find_or_fetch_dependency(
    spdlog
    "https://github.com/gabime/spdlog.git"
    "v1.12.0"
    "spdlog"
)

if(USE_PUGIXML)
    find_or_fetch_dependency(
        pugixml
        "https://github.com/zeux/pugixml.git"
        "v1.14"
        "pugixml"
    )
endif()

# Find GTest only if tests are enabled
if(BUILD_TESTS)
    find_or_fetch_dependency(
        googletest
        "https://github.com/google/googletest.git"
        "v1.14.0"
        "GTest"
    )

    # If we fetched GTest via FetchContent, ensure proper targets
    if(NOT GTest_FOUND AND TARGET gtest)
        # For FetchContent GTest, create aliases that match find_package behavior
        if(NOT TARGET GTest::gtest)
            add_library(GTest::gtest ALIAS gtest)
        endif()
        if(NOT TARGET GTest::gtest_main)
            add_library(GTest::gtest_main ALIAS gtest_main)
        endif()
        if(NOT TARGET GTest::gmock)
            add_library(GTest::gmock ALIAS gmock)
        endif()
        if(NOT TARGET GTest::gmock_main)
            add_library(GTest::gmock_main ALIAS gmock_main)
        endif()

        # Set GTest found flag for proper handling in tests
        set(GTest_FOUND TRUE)
    endif()
endif()

# Find system libraries
if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(X11 REQUIRED x11)
    pkg_check_modules(XTST REQUIRED xtst)
    pkg_check_modules(XI REQUIRED xi)
elseif(WIN32)
    # Windows libraries are linked automatically
endif()

# Add subdirectories
add_subdirectory(src)

# Enable testing only if BUILD_TESTS is ON
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
