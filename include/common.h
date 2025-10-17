#pragma once

#include <string>
#include <memory>
#include <vector>
#include <chrono>

namespace playrec {

// Common types and constants
using TimeStamp = std::chrono::high_resolution_clock::time_point;
using TimeDuration = std::chrono::duration<double>;

// Video frame format
enum class VideoFormat {
    RGB24,
    RGBA32,
    BGR24,
    BGRA32,
    YUV420P
};

// Audio format
enum class AudioFormat {
    PCM_S16LE,
    PCM_S24LE,
    PCM_S32LE,
    PCM_F32LE
};

// Capture quality settings
enum class Quality {
    LOW,
    MEDIUM,
    HIGH,
    ULTRA
};

// Frame data structure
struct Frame {
    std::vector<uint8_t> data;
    int width;
    int height;
    VideoFormat format;
    TimeStamp timestamp;
};

// Audio sample structure
struct AudioSample {
    std::vector<uint8_t> data;
    int sample_rate;
    int channels;
    AudioFormat format;
    TimeStamp timestamp;
};

// Capture settings
struct CaptureSettings {
    int target_fps = 60;
    Quality quality = Quality::HIGH;
    bool capture_audio = true;
    bool capture_cursor = true;
    std::string output_path = "capture.mp4";
    std::string codec = "h264";
};

} // namespace playrec