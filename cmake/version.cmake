# Version generation for MouseRecorder
# This module generates version.hpp from version.hpp.in

# Get git information if available
find_package(Git QUIET)

# Initialize version variables with defaults
set(PROJECT_VERSION_FULL "${PROJECT_VERSION}")
set(GIT_HASH "unknown")
set(GIT_BRANCH "unknown")
set(GIT_DIRTY "false")

if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    message(STATUS "Git found, extracting version information...")

    # Get git commit hash
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Get git branch name
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Check if working tree is dirty
    execute_process(
        COMMAND ${GIT_EXECUTABLE} diff-index --quiet HEAD --
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE GIT_DIRTY_RESULT
        ERROR_QUIET
    )

    if(GIT_DIRTY_RESULT)
        set(GIT_DIRTY "true")
        set(PROJECT_VERSION_FULL "${PROJECT_VERSION}-dirty")
    else()
        set(GIT_DIRTY "false")
    endif()

    # Try to get version from git tags
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --exact-match HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE GIT_TAG_RESULT
    )

    if(GIT_TAG_RESULT EQUAL 0)
        # We're on a tagged commit, use the tag as version
        set(PROJECT_VERSION_FULL "${GIT_TAG}")
        if(GIT_DIRTY STREQUAL "true")
            set(PROJECT_VERSION_FULL "${PROJECT_VERSION_FULL}-dirty")
        endif()
        message(STATUS "Using git tag version: ${PROJECT_VERSION_FULL}")
    else()
        # Not on a tagged commit, include commit hash
        if(GIT_DIRTY STREQUAL "true")
            set(PROJECT_VERSION_FULL "${PROJECT_VERSION}-g${GIT_HASH}-dirty")
        else()
            set(PROJECT_VERSION_FULL "${PROJECT_VERSION}-g${GIT_HASH}")
        endif()
        message(STATUS "Using development version: ${PROJECT_VERSION_FULL}")
    endif()
else()
    message(STATUS "Git not found or not a git repository, using project version only")
endif()

# Handle missing PROJECT_VERSION_TWEAK
if(NOT DEFINED PROJECT_VERSION_TWEAK OR PROJECT_VERSION_TWEAK STREQUAL "")
    set(PROJECT_VERSION_TWEAK 0)
endif()

# Get current timestamp
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)

# Configure the version header
configure_file(
    "${CMAKE_SOURCE_DIR}/src/version.hpp.in"
    "${CMAKE_BINARY_DIR}/generated/version.hpp"
    @ONLY
)

# Configure the desktop file
configure_file(
    "${CMAKE_SOURCE_DIR}/mouserecorder.desktop.in"
    "${CMAKE_BINARY_DIR}/mouserecorder.desktop"
    @ONLY
)

# Add the generated directory to include path
include_directories("${CMAKE_BINARY_DIR}/generated")

# Display version information
message(STATUS "MouseRecorder Version: ${PROJECT_VERSION_FULL}")
message(STATUS "Git Hash: ${GIT_HASH}")
message(STATUS "Git Branch: ${GIT_BRANCH}")
message(STATUS "Git Dirty: ${GIT_DIRTY}")
message(STATUS "Build Timestamp: ${BUILD_TIMESTAMP}")
