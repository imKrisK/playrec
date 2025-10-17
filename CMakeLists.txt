cmake_minimum_required(VERSION 3.16)
project(PlayRec VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

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
    src/main.cpp
    src/capture_engine.cpp
    src/video_capture.cpp
    src/audio_capture.cpp
    src/encoder.cpp
    src/file_writer.cpp
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

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
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

# Install rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)