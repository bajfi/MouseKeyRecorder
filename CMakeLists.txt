cmake_minimum_required(VERSION 3.20)

project(MouseRecorder
    VERSION 1.0.0
    DESCRIPTION "Cross-platform Mouse and Keyboard Event Recorder"
    LANGUAGES CXX
)

# Set C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable modern C++ features and warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find required packages
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Test Gui)
find_package(PkgConfig REQUIRED)

# Try to find nlohmann_json, fallback to pkg-config if not found
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    pkg_check_modules(nlohmann_json REQUIRED nlohmann_json)
endif()

# Find spdlog
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
    pkg_check_modules(spdlog REQUIRED spdlog)
endif()

# Find pugixml
find_package(pugixml QUIET)
if(NOT pugixml_FOUND)
    pkg_check_modules(pugixml REQUIRED pugixml)
endif()

# Find GTest
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    pkg_check_modules(GTest REQUIRED gtest)
endif()

# Find system libraries
if(UNIX AND NOT APPLE)
    pkg_check_modules(X11 REQUIRED x11)
    pkg_check_modules(XTST REQUIRED xtst)
    pkg_check_modules(XI REQUIRED xi)
elseif(WIN32)
    # Windows libraries are linked automatically
endif()

# Enable Qt MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Set UIC search paths
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/src/gui/ui)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)

# Add subdirectories
add_subdirectory(src)

# Enable testing
enable_testing()
add_subdirectory(tests)
