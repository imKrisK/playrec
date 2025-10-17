#pragma once

#include "common.h"
#include "video_capture.h"
#include "audio_capture.h"
#include "encoder.h"
#include "file_writer.h"
#include <memory>
#include <thread>
#include <atomic>

namespace playrec {

class CaptureEngine {
public:
    CaptureEngine();
    ~CaptureEngine();

    // Initialize the capture engine with settings
    bool initialize(const CaptureSettings& settings);

    // Start capturing
    bool start_capture();

    // Stop capturing
    void stop_capture();

    // Check if currently capturing
    bool is_capturing() const;

    // Get capture statistics
    struct Stats {
        uint64_t frames_captured = 0;
        uint64_t frames_dropped = 0;
        double average_fps = 0.0;
        double cpu_usage = 0.0;
        uint64_t file_size_bytes = 0;
    };
    
    Stats get_stats() const;

private:
    void capture_loop();
    void process_video_frame(const Frame& frame);
    void process_audio_sample(const AudioSample& sample);

    CaptureSettings m_settings;
    std::unique_ptr<VideoCapture> m_video_capture;
    std::unique_ptr<AudioCapture> m_audio_capture;
    std::unique_ptr<Encoder> m_encoder;
    std::unique_ptr<FileWriter> m_file_writer;

    std::thread m_capture_thread;
    std::atomic<bool> m_is_capturing{false};
    std::atomic<bool> m_should_stop{false};

    mutable Stats m_stats;
    TimeStamp m_start_time;
};

} // namespace playrec