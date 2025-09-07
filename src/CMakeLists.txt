# Source CMakeLists.txt

# Core library sources and headers
set(CORE_SOURCES
    core/Event.cpp
    core/QtConfiguration.cpp
    core/MouseMovementOptimizer.cpp
)

set(CORE_HEADERS
    core/Event.hpp
    core/IEventRecorder.hpp
    core/IEventPlayer.hpp
    core/IEventStorage.hpp
    core/IConfiguration.hpp
    core/QtConfiguration.hpp
    core/MouseMovementOptimizer.hpp
)

# Conditionally add nlohmann::json-based Configuration
if(USE_NLOHMANN_JSON)
    list(APPEND CORE_SOURCES core/Configuration.cpp)
    list(APPEND CORE_HEADERS core/Configuration.hpp)
endif()

# Serialization sources and headers
set(SERIALIZATION_SOURCES
    core/serialization/QtJsonEventSerializer.cpp
    core/serialization/QtXmlEventSerializer.cpp
    core/serialization/EventSerializerFactory.cpp
)

set(SERIALIZATION_HEADERS
    core/serialization/IEventSerializer.hpp
    core/serialization/QtJsonEventSerializer.hpp
    core/serialization/QtXmlEventSerializer.hpp
    core/serialization/EventSerializerFactory.hpp
)

# Add third-party serializers based on options
if(USE_NLOHMANN_JSON)
    list(APPEND SERIALIZATION_SOURCES core/serialization/NlohmannJsonEventSerializer.cpp)
    list(APPEND SERIALIZATION_HEADERS core/serialization/NlohmannJsonEventSerializer.hpp)
endif()

if(USE_PUGIXML)
    list(APPEND SERIALIZATION_SOURCES core/serialization/PugixmlEventSerializer.cpp)
    list(APPEND SERIALIZATION_HEADERS core/serialization/PugixmlEventSerializer.hpp)
endif()

# Add serialization to core sources
list(APPEND CORE_SOURCES ${SERIALIZATION_SOURCES})
list(APPEND CORE_HEADERS ${SERIALIZATION_HEADERS})

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

# Add the generated version header as a dependency
add_custom_target(GenerateVersion ALL
    DEPENDS "${CMAKE_BINARY_DIR}/generated/version.hpp"
)

# Make sure libraries depend on version generation
add_dependencies(MouseRecorderCore GenerateVersion)

# Set serialization preprocessor definitions
if(USE_NLOHMANN_JSON)
    target_compile_definitions(MouseRecorderCore PRIVATE MOUSERECORDER_USE_NLOHMANN_JSON)
endif()

if(USE_PUGIXML)
    target_compile_definitions(MouseRecorderCore PRIVATE MOUSERECORDER_USE_PUGIXML)
endif()

if(DEFAULT_QT_SERIALIZATION)
    target_compile_definitions(MouseRecorderCore PRIVATE MOUSERECORDER_DEFAULT_QT_SERIALIZATION)
endif()

# Create GUI library
add_library(MouseRecorderGUI STATIC
    ${GUI_SOURCES}
    ${GUI_HEADERS}
    ${GUI_UI_FILES}
    ${GUI_RESOURCE_FILES}
)

# Make sure GUI library also depends on version generation
add_dependencies(MouseRecorderGUI GenerateVersion)

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
        Qt${QT_VERSION_MAJOR}::Xml
)

target_link_libraries(MouseRecorderGUI
    PUBLIC
        MouseRecorderCore
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Gui
)

# Link third-party dependencies based on options
# spdlog
if(TARGET spdlog::spdlog)
    target_link_libraries(MouseRecorderCore PUBLIC spdlog::spdlog)
elseif(spdlog_FOUND)
    target_link_libraries(MouseRecorderCore PUBLIC ${spdlog_LIBRARIES})
    target_include_directories(MouseRecorderCore PUBLIC ${spdlog_INCLUDE_DIRS})
endif()

# nlohmann_json (conditionally)
if(USE_NLOHMANN_JSON)
    if(TARGET nlohmann_json::nlohmann_json)
        target_link_libraries(MouseRecorderCore PUBLIC nlohmann_json::nlohmann_json)
    elseif(nlohmann_json_FOUND)
        target_include_directories(MouseRecorderCore PUBLIC ${nlohmann_json_INCLUDE_DIRS})
    endif()
endif()

# pugixml (conditionally)
if(USE_PUGIXML)
    if(TARGET pugixml::pugixml)
        target_link_libraries(MouseRecorderCore PUBLIC pugixml::pugixml)
    elseif(TARGET pugixml)
        target_link_libraries(MouseRecorderCore PUBLIC pugixml)
    elseif(pugixml_FOUND)
        target_link_libraries(MouseRecorderCore PUBLIC ${pugixml_LIBRARIES})
        target_include_directories(MouseRecorderCore PUBLIC ${pugixml_INCLUDE_DIRS})
    endif()
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
    target_link_libraries(MouseRecorderCore PUBLIC
        user32
        kernel32
        gdi32
        shell32
        ole32
        oleaut32
        uuid
        comdlg32
        advapi32
    )
endif()

# Main executable
add_executable(MouseRecorder main.cpp)
target_link_libraries(MouseRecorder PRIVATE MouseRecorderGUI)

# Make sure the executable also depends on version generation
add_dependencies(MouseRecorder GenerateVersion)

# Set target properties for executable
set_target_properties(MouseRecorder PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
