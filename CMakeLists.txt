cmake_minimum_required(VERSION 3.16)

project(MouseRecorder
    VERSION 0.0.1
    DESCRIPTION "Cross-platform Mouse and Keyboard Event Recorder"
    LANGUAGES CXX
)

# Options
option(BUILD_TESTS "Build tests" OFF)
option(USE_SYSTEM_DEPS "Use system dependencies instead of fetching" ON)

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
find_package(Qt6 QUIET COMPONENTS Core Widgets Test Gui)
if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
    message(STATUS "Using Qt6")
else()
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets Test Gui)
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

# Find or fetch third-party dependencies
find_or_fetch_dependency(
    nlohmann_json
    "https://github.com/nlohmann/json.git"
    "v3.11.3"
    "nlohmann_json"
)

find_or_fetch_dependency(
    spdlog
    "https://github.com/gabime/spdlog.git"
    "v1.12.0"
    "spdlog"
)

find_or_fetch_dependency(
    pugixml
    "https://github.com/zeux/pugixml.git"
    "v1.14"
    "pugixml"
)

# Find GTest only if tests are enabled
if(BUILD_TESTS)
    find_or_fetch_dependency(
        gtest
        "https://github.com/google/googletest.git"
        "v1.14.0"
        "GTest"
    )
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
