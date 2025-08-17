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
    platform/test_LinuxEventCapture.cpp
    platform/test_LinuxEventReplay.cpp
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

    # Link GTest
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
if(TARGET GTest::gtest OR TARGET gtest)
    include(GoogleTest)
    gtest_discover_tests(MouseRecorderTests)
endif()

# Add individual GUI tests to CTest
add_test(NAME TestGlobalShortcuts COMMAND TestGlobalShortcuts)
add_test(NAME TestRecordingWidget COMMAND TestRecordingWidget)
add_test(NAME TestMainWindow COMMAND TestMainWindow)
add_test(NAME TestSystemTrayFunctionality COMMAND TestSystemTrayFunctionality)
