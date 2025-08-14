# Source CMakeLists.txt

# Core library
set(CORE_SOURCES
    core/Event.cpp
    core/Configuration.cpp
    core/QtConfiguration.cpp
)

set(CORE_HEADERS
    core/Event.hpp
    core/IEventRecorder.hpp
    core/IEventPlayer.hpp
    core/IEventStorage.hpp
    core/IConfiguration.hpp
    core/Configuration.hpp
    core/QtConfiguration.hpp
)

# Platform-specific sources
if(WIN32)
    set(PLATFORM_SOURCES
        platform/windows/WindowsEventCapture.cpp
        platform/windows/WindowsEventReplay.cpp
    )
    set(PLATFORM_HEADERS
        platform/windows/WindowsEventCapture.hpp
        platform/windows/WindowsEventReplay.hpp
    )
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_SOURCES
        platform/linux/LinuxEventCapture.cpp
        platform/linux/LinuxEventReplay.cpp
    )
    set(PLATFORM_HEADERS
        platform/linux/LinuxEventCapture.hpp
        platform/linux/LinuxEventReplay.hpp
    )
endif()

# Storage sources
set(STORAGE_SOURCES
    storage/JsonEventStorage.cpp
    storage/XmlEventStorage.cpp
    storage/BinaryEventStorage.cpp
    storage/EventStorageFactory.cpp
)

set(STORAGE_HEADERS
    storage/JsonEventStorage.hpp
    storage/XmlEventStorage.hpp
    storage/BinaryEventStorage.hpp
    storage/EventStorageFactory.hpp
)

# Application sources
set(APP_SOURCES
    application/MouseRecorderApp.cpp
)

set(APP_HEADERS
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
    ${PLATFORM_SOURCES}
    ${PLATFORM_HEADERS}
    ${STORAGE_SOURCES}
    ${STORAGE_HEADERS}
    ${APP_SOURCES}
    ${APP_HEADERS}
)

# Create GUI library
add_library(MouseRecorderGUI STATIC
    ${GUI_SOURCES}
    ${GUI_HEADERS}
    ${GUI_UI_FILES}
    ${GUI_RESOURCE_FILES}
)

# Link libraries
target_link_libraries(MouseRecorderCore
    Qt5::Core
)

# Link spdlog
if(TARGET spdlog::spdlog)
    target_link_libraries(MouseRecorderCore spdlog::spdlog)
elseif(spdlog_FOUND)
    target_link_libraries(MouseRecorderCore ${spdlog_LIBRARIES})
    target_include_directories(MouseRecorderCore PRIVATE ${spdlog_INCLUDE_DIRS})
endif()

# Link nlohmann_json
if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(MouseRecorderCore nlohmann_json::nlohmann_json)
elseif(nlohmann_json_FOUND)
    target_include_directories(MouseRecorderCore PRIVATE ${nlohmann_json_INCLUDE_DIRS})
endif()

# Link pugixml
if(TARGET pugixml::pugixml)
    target_link_libraries(MouseRecorderCore pugixml::pugixml)
elseif(pugixml_FOUND)
    target_link_libraries(MouseRecorderCore ${pugixml_LIBRARIES})
    target_include_directories(MouseRecorderCore PRIVATE ${pugixml_INCLUDE_DIRS})
endif()

target_link_libraries(MouseRecorderGUI
    MouseRecorderCore
    Qt5::Widgets
    Qt5::Gui
)

# Platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(MouseRecorderCore 
        ${X11_LIBRARIES} 
        ${XTST_LIBRARIES} 
        ${XI_LIBRARIES}
    )
    target_include_directories(MouseRecorderCore PRIVATE 
        ${X11_INCLUDE_DIRS} 
        ${XTST_INCLUDE_DIRS} 
        ${XI_INCLUDE_DIRS}
    )
elseif(WIN32)
    target_link_libraries(MouseRecorderCore user32)
endif()

# Main executable
add_executable(MouseRecorder main.cpp)
target_link_libraries(MouseRecorder MouseRecorderGUI)

# Set target properties
set_target_properties(MouseRecorderCore PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(MouseRecorderGUI PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(MouseRecorder PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)
