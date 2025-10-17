cmake_minimum_required(VERSION 3.16)
project(PlayRec VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia MultimediaWidgets)

# Find FFmpeg components
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavdevice
    libavformat
    libavfilter
    libavcodec
    libswresample
    libswscale
    libavutil
)

# Platform-specific libraries
if(WIN32)
    # Windows specific libraries for screen capture
    set(PLATFORM_LIBS d3d11 dxgi user32 gdi32)
elseif(APPLE)
    # macOS specific frameworks
    find_library(COREGRAPHICS_LIBRARY CoreGraphics REQUIRED)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
    find_library(IMAGEIO_LIBRARY ImageIO REQUIRED)
    find_library(AVFOUNDATION_LIBRARY AVFoundation REQUIRED)
    find_library(COREMEDIA_LIBRARY CoreMedia REQUIRED)
    find_library(COREAUDIO_LIBRARY CoreAudio REQUIRED)
    find_library(AUDIOUNIT_LIBRARY AudioUnit REQUIRED)
    set(PLATFORM_LIBS 
        ${COREGRAPHICS_LIBRARY} 
        ${COREFOUNDATION_LIBRARY}
        ${IMAGEIO_LIBRARY}
        ${AVFOUNDATION_LIBRARY} 
        ${COREMEDIA_LIBRARY}
        ${COREAUDIO_LIBRARY}
        ${AUDIOUNIT_LIBRARY}
    )
elseif(UNIX)
    # Linux specific libraries (X11, V4L2)
    find_package(X11 REQUIRED)
    set(PLATFORM_LIBS ${X11_LIBRARIES})
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/capture_engine.cpp
    src/video_capture.cpp
    src/audio_capture.cpp
    src/encoder.cpp
    src/file_writer.cpp
)

# GUI Application sources
set(GUI_SOURCES
    src/gui/main_gui.cpp
    src/gui/main_window.cpp
    src/gui/preview_widget.cpp
    src/gui/settings_dialog.cpp
    src/gui/capture_thread.cpp
)

# Command-line Application sources  
set(CLI_SOURCES
    src/main.cpp
)

# Header files
set(HEADERS
    include/capture_engine.h
    include/video_capture.h
    include/audio_capture.h
    include/encoder.h
    include/file_writer.h
    include/common.h
)

# GUI Header files
set(GUI_HEADERS
    include/gui/main_window.h
    include/gui/preview_widget.h
    include/gui/settings_dialog.h
    include/gui/capture_thread.h
)

# Enable Qt MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Create GUI executable
add_executable(${PROJECT_NAME}-gui ${GUI_SOURCES} ${SOURCES} ${GUI_HEADERS} ${HEADERS})

# Create CLI executable  
add_executable(${PROJECT_NAME} ${CLI_SOURCES} ${SOURCES} ${HEADERS})

# Link libraries for GUI
target_link_libraries(${PROJECT_NAME}-gui 
    ${PLATFORM_LIBS}
    Threads::Threads
    PkgConfig::LIBAV
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
)

# Link libraries for CLI
target_link_libraries(${PROJECT_NAME} 
    ${PLATFORM_LIBS}
    Threads::Threads
    PkgConfig::LIBAV
)

# Compiler-specific options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
    # For macOS, allow deprecated APIs for backward compatibility
    if(APPLE)
        target_compile_options(${PROJECT_NAME} PRIVATE -Wno-deprecated-declarations)
    endif()
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set_target_properties(${PROJECT_NAME}-gui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install rules
install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}-gui
    RUNTIME DESTINATION bin
)