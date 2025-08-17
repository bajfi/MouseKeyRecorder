# Source CMakeLists.txt

# Core library sources and headers
set(CORE_SOURCES
    core/Event.cpp
    core/Configuration.cpp
    core/QtConfiguration.cpp
    core/MouseMovementOptimizer.cpp
)

set(CORE_HEADERS
    core/Event.hpp
    core/IEventRecorder.hpp
    core/IEventPlayer.hpp
    core/IEventStorage.hpp
    core/IConfiguration.hpp
    core/Configuration.hpp
    core/QtConfiguration.hpp
    core/MouseMovementOptimizer.hpp
)

# Platform-specific sources
if(WIN32)
    list(APPEND CORE_SOURCES
        platform/windows/WindowsEventCapture.cpp
        platform/windows/WindowsEventReplay.cpp
    )
    list(APPEND CORE_HEADERS
        platform/windows/WindowsEventCapture.hpp
        platform/windows/WindowsEventReplay.hpp
    )
elseif(UNIX AND NOT APPLE)
    list(APPEND CORE_SOURCES
        platform/linux/LinuxEventCapture.cpp
        platform/linux/LinuxEventReplay.cpp
    )
    list(APPEND CORE_HEADERS
        platform/linux/LinuxEventCapture.hpp
        platform/linux/LinuxEventReplay.hpp
    )
endif()

# Storage sources
list(APPEND CORE_SOURCES
    storage/JsonEventStorage.cpp
    storage/XmlEventStorage.cpp
    storage/BinaryEventStorage.cpp
    storage/EventStorageFactory.cpp
)

list(APPEND CORE_HEADERS
    storage/JsonEventStorage.hpp
    storage/XmlEventStorage.hpp
    storage/BinaryEventStorage.hpp
    storage/EventStorageFactory.hpp
)

# Application sources
list(APPEND CORE_SOURCES
    application/MouseRecorderApp.cpp
)

list(APPEND CORE_HEADERS
    application/MouseRecorderApp.hpp
)

# GUI sources
set(GUI_SOURCES
    gui/MainWindow.cpp
    gui/RecordingWidget.cpp
    gui/PlaybackWidget.cpp
    gui/ConfigurationWidget.cpp
)

set(GUI_HEADERS
    gui/MainWindow.hpp
    gui/RecordingWidget.hpp
    gui/PlaybackWidget.hpp
    gui/ConfigurationWidget.hpp
)

set(GUI_UI_FILES
    gui/ui/MainWindow.ui
    gui/ui/RecordingWidget.ui
    gui/ui/PlaybackWidget.ui
    gui/ui/ConfigurationWidget.ui
)

# GUI resource files
set(GUI_RESOURCE_FILES
    resources/icons.qrc
)

# Create core library
add_library(MouseRecorderCore STATIC
    ${CORE_SOURCES}
    ${CORE_HEADERS}
)

# Create GUI library
add_library(MouseRecorderGUI STATIC
    ${GUI_SOURCES}
    ${GUI_HEADERS}
    ${GUI_UI_FILES}
    ${GUI_RESOURCE_FILES}
)

# Set target properties
set_target_properties(MouseRecorderCore MouseRecorderGUI PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Include directories for MouseRecorderCore
target_include_directories(MouseRecorderCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
# Link Qt libraries (version-agnostic)
target_link_libraries(MouseRecorderCore
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
)

target_link_libraries(MouseRecorderGUI
    PUBLIC
        MouseRecorderCore
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Gui
)

# Link third-party dependencies
# spdlog
if(TARGET spdlog::spdlog)
    target_link_libraries(MouseRecorderCore PUBLIC spdlog::spdlog)
elseif(spdlog_FOUND)
    target_link_libraries(MouseRecorderCore PUBLIC ${spdlog_LIBRARIES})
    target_include_directories(MouseRecorderCore PUBLIC ${spdlog_INCLUDE_DIRS})
endif()

# nlohmann_json
if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(MouseRecorderCore PUBLIC nlohmann_json::nlohmann_json)
elseif(nlohmann_json_FOUND)
    target_include_directories(MouseRecorderCore PUBLIC ${nlohmann_json_INCLUDE_DIRS})
endif()

# pugixml
if(TARGET pugixml::pugixml)
    target_link_libraries(MouseRecorderCore PUBLIC pugixml::pugixml)
elseif(TARGET pugixml)
    target_link_libraries(MouseRecorderCore PUBLIC pugixml)
elseif(pugixml_FOUND)
    target_link_libraries(MouseRecorderCore PUBLIC ${pugixml_LIBRARIES})
    target_include_directories(MouseRecorderCore PUBLIC ${pugixml_INCLUDE_DIRS})
endif()

# Platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(MouseRecorderCore
        PUBLIC
            ${X11_LIBRARIES}
            ${XTST_LIBRARIES}
            ${XI_LIBRARIES}
    )
    target_include_directories(MouseRecorderCore
        PUBLIC
            ${X11_INCLUDE_DIRS}
            ${XTST_INCLUDE_DIRS}
            ${XI_INCLUDE_DIRS}
    )
elseif(WIN32)
    target_link_libraries(MouseRecorderCore PUBLIC user32)
endif()

# Main executable
add_executable(MouseRecorder main.cpp)
target_link_libraries(MouseRecorder PRIVATE MouseRecorderGUI)

# Set target properties for executable
set_target_properties(MouseRecorder PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
