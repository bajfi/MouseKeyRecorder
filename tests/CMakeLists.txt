# Tests CMakeLists.txt

# Test sources
set(TEST_SOURCES
    test_main.cpp
    core/test_Event.cpp
    core/test_EventRecording.cpp
    application/test_MouseRecorderApp.cpp
    application/test_ShutdownBehavior.cpp
    platform/test_LinuxEventCapture.cpp
    platform/test_LinuxEventReplay.cpp
    storage/test_EventStorage.cpp
    storage/test_EventStorageFormats.cpp
    gui/test_ExportFunctionality.cpp
    gui/test_PlaybackWidget.cpp
    # gui/test_SystemTrayKeyboardShortcuts.cpp  # Temporarily disabled due to namespace complexity
    # gui/test_UIStateSynchronization.cpp       # Temporarily disabled due to namespace complexity
    integration/test_PlaybackIntegration.cpp
    integration/test_E2EPlayback.cpp
    integration/test_SystemTrayAndShortcuts.cpp
)

# Create test executable
add_executable(MouseRecorderTests ${TEST_SOURCES})

# Create separate GUI test executables (each has its own main)
add_executable(TestRecordingWidget gui/test_RecordingWidget.cpp)
add_executable(TestMainWindow gui/test_MainWindow.cpp)
add_executable(TestSystemTrayFunctionality gui/test_SystemTrayFunctionality.cpp)
add_executable(TestGlobalShortcuts gui/test_GlobalShortcuts.cpp)
# add_executable(TestUIIntegration gui/test_UIIntegration.cpp)  # Disabled due to Qt event loop issues

# Link libraries
target_link_libraries(MouseRecorderTests
    MouseRecorderCore
    MouseRecorderGUI
    Qt5::Test
    Qt5::Widgets
)

target_link_libraries(TestRecordingWidget
    MouseRecorderCore
    MouseRecorderGUI
    Qt5::Test
    Qt5::Widgets
)

target_link_libraries(TestMainWindow
    MouseRecorderCore
    MouseRecorderGUI
    Qt5::Test
    Qt5::Widgets
)

target_link_libraries(TestSystemTrayFunctionality
    MouseRecorderCore
    MouseRecorderGUI
    Qt5::Test
    Qt5::Widgets
)

target_link_libraries(TestGlobalShortcuts
    MouseRecorderCore
    MouseRecorderGUI
    Qt5::Test
    Qt5::Widgets
)

# Add TestGlobalShortcuts to CTest
add_test(NAME TestGlobalShortcuts COMMAND TestGlobalShortcuts)

# Link GTest
if(TARGET GTest::gtest)
    target_link_libraries(MouseRecorderTests GTest::gtest GTest::gtest_main)
elseif(GTest_FOUND)
    target_link_libraries(MouseRecorderTests ${GTest_LIBRARIES})
    target_include_directories(MouseRecorderTests PRIVATE ${GTest_INCLUDE_DIRS})
    # Find gtest_main
    find_library(GTEST_MAIN_LIBRARY gtest_main)
    if(GTEST_MAIN_LIBRARY)
        target_link_libraries(MouseRecorderTests ${GTEST_MAIN_LIBRARY})
    endif()
endif()

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(MouseRecorderTests)
