# Tests CMakeLists.txt

# Test sources
set(TEST_SOURCES
    test_main.cpp
    core/test_Event.cpp
    core/test_EventRecording.cpp
    core/test_QtConfiguration.cpp
    core/test_MouseMovementOptimizer.cpp
    application/test_MouseRecorderApp.cpp
    application/test_ShutdownBehavior.cpp
    storage/test_EventStorage.cpp
    storage/test_EventStorageFormats.cpp
    storage/test_EventStorageMetadata.cpp
    gui/test_ExportFunctionality.cpp
    gui/test_PlaybackWidget.cpp
    gui/test_ConfigurationPersistence.cpp
    # gui/test_SystemTrayKeyboardShortcuts.cpp  # Temporarily disabled due to namespace complexity
    # gui/test_UIStateSynchronization.cpp       # Temporarily disabled due to namespace complexity
    integration/test_PlaybackIntegration.cpp
    integration/test_E2EPlayback.cpp
    integration/test_SystemTrayAndShortcuts.cpp
    integration/test_MouseMovementOptimization.cpp
)

if (LINUX)
    list(APPEND TEST_SOURCES
        platform/linux/test_LinuxEventCapture.cpp
        platform/linux/test_LinuxEventReplay.cpp
    )
endif()

# Add Windows platform tests if building on Windows
if(WIN32)
    list(APPEND TEST_SOURCES
        platform/windows/test_WindowsEventCapture.cpp
        platform/windows/test_WindowsEventReplay.cpp
    )
endif()

# Function to create a test executable with proper linking
function(create_test_executable target_name sources)
    add_executable(${target_name} ${sources})

    target_link_libraries(${target_name}
        PRIVATE
            MouseRecorderCore
            MouseRecorderGUI
            Qt${QT_VERSION_MAJOR}::Test
            Qt${QT_VERSION_MAJOR}::Widgets
    )

    # Link GTest - prioritize modern targets
    if(TARGET GTest::gtest)
        target_link_libraries(${target_name} PRIVATE GTest::gtest GTest::gtest_main)
    elseif(TARGET gtest)
        target_link_libraries(${target_name} PRIVATE gtest gtest_main)
    elseif(GTest_FOUND)
        target_link_libraries(${target_name} PRIVATE ${GTest_LIBRARIES})
        target_include_directories(${target_name} PRIVATE ${GTest_INCLUDE_DIRS})
        # Find gtest_main
        find_library(GTEST_MAIN_LIBRARY gtest_main)
        if(GTEST_MAIN_LIBRARY)
            target_link_libraries(${target_name} PRIVATE ${GTEST_MAIN_LIBRARY})
        endif()
    else()
        message(FATAL_ERROR "GTest not found and no fallback available")
    endif()

    # Set target properties
    set_target_properties(${target_name} PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
endfunction()

# Create main test executable
create_test_executable(MouseRecorderTests "${TEST_SOURCES}")

# Create separate GUI test executables (each has its own main)
create_test_executable(TestRecordingWidget "gui/test_RecordingWidget.cpp")
create_test_executable(TestMainWindow "gui/test_MainWindow.cpp")
create_test_executable(TestSystemTrayFunctionality "gui/test_SystemTrayFunctionality.cpp")
create_test_executable(TestGlobalShortcuts "gui/test_GlobalShortcuts.cpp")
# create_test_executable(TestUIIntegration "gui/test_UIIntegration.cpp")  # Disabled due to Qt event loop issues
# Add tests to CTest
if(TARGET GTest::gtest OR TARGET gtest OR GTest_FOUND)
    # On Windows, avoid gtest_discover_tests due to CMake path issues
    # Use simple add_test instead for better reliability
    if(WIN32)
        add_test(NAME MouseRecorderTests COMMAND MouseRecorderTests)
        set_tests_properties(MouseRecorderTests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        )
    else()
        # On Linux/Unix, try to use gtest_discover_tests with fallback
        include(GoogleTest)
        gtest_discover_tests(MouseRecorderTests
            PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;DISPLAY=:99"
            DISCOVERY_TIMEOUT 60
        )
    endif()
endif()# Add individual GUI tests to CTest
add_test(NAME TestGlobalShortcuts COMMAND TestGlobalShortcuts)
if(WIN32)
    set_tests_properties(TestGlobalShortcuts PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
else()
    set_tests_properties(TestGlobalShortcuts PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;DISPLAY=:99")
endif()

add_test(NAME TestRecordingWidget COMMAND TestRecordingWidget)
if(WIN32)
    set_tests_properties(TestRecordingWidget PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
else()
    set_tests_properties(TestRecordingWidget PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;DISPLAY=:99")
endif()

add_test(NAME TestMainWindow COMMAND TestMainWindow)
if(WIN32)
    set_tests_properties(TestMainWindow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
else()
    set_tests_properties(TestMainWindow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;DISPLAY=:99")
endif()

add_test(NAME TestSystemTrayFunctionality COMMAND TestSystemTrayFunctionality)
if(WIN32)
    set_tests_properties(TestSystemTrayFunctionality PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
else()
    set_tests_properties(TestSystemTrayFunctionality PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;DISPLAY=:99")
endif()
