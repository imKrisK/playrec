#include "capture_engine.h"
#include <iostream>
#include <chrono>

namespace playrec {

CaptureEngine::CaptureEngine() = default;

CaptureEngine::~CaptureEngine() {
    if (m_is_capturing) {
        stop_capture();
    }
}

bool CaptureEngine::initialize(const CaptureSettings& settings) {
    m_settings = settings;

    try {
        // Create platform-specific video capture
        m_video_capture = create_video_capture();
        if (!m_video_capture || !m_video_capture->initialize(settings)) {
            std::cerr << "Failed to initialize video capture\n";
            return false;
        }

        // Create audio capture if enabled
        if (settings.capture_audio) {
            m_audio_capture = create_audio_capture();
            if (!m_audio_capture || !m_audio_capture->initialize(settings)) {
                std::cerr << "Failed to initialize audio capture\n";
                return false;
            }
        }

        // Get video resolution for encoder setup
        auto [width, height] = m_video_capture->get_resolution();

        // Create encoder
        m_encoder = create_encoder(settings.codec);
        if (!m_encoder) {
            std::cerr << "Failed to create encoder\n";
            return false;
        }

        // Initialize encoder
        AudioFormat audio_format = AudioFormat::PCM_S16LE;
        int sample_rate = 44100, channels = 2;
        if (m_audio_capture) {
            audio_format = m_audio_capture->get_format();
            sample_rate = m_audio_capture->get_sample_rate();
            channels = m_audio_capture->get_channels();
        }

        if (!m_encoder->initialize(settings, width, height, audio_format, sample_rate, channels)) {
            std::cerr << "Failed to initialize encoder\n";
            return false;
        }

        // Create MP4 writer for proper container
        m_mp4_writer = std::make_unique<MP4Writer>();
        if (!m_mp4_writer->initialize(settings.output_path, width, height, settings.target_fps, 
                                     sample_rate, channels)) {
            std::cerr << "Failed to initialize MP4 writer for: " << settings.output_path << "\n";
            return false;
        }

        // Set up callbacks
        m_video_capture->set_frame_callback([this](const Frame& frame) {
            process_video_frame(frame);
        });

        if (m_audio_capture) {
            m_audio_capture->set_sample_callback([this](const AudioSample& sample) {
                process_audio_sample(sample);
            });
        }

        std::cout << "Capture engine initialized:\n";
        std::cout << "  Video: " << width << "x" << height << " @ " << settings.target_fps << " FPS\n";
        std::cout << "  Audio: " << (settings.capture_audio ? "Enabled" : "Disabled") << "\n";
        std::cout << "  Encoder: " << m_encoder->get_codec_name() << "\n";
        std::cout << "  HW Acceleration: " << (m_encoder->supports_hardware_acceleration() ? "Yes" : "No") << "\n";

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception during initialization: " << e.what() << "\n";
        return false;
    }
}

bool CaptureEngine::start_capture() {
    if (m_is_capturing) {
        return false;
    }

    m_should_stop = false;
    m_stats = Stats{}; // Reset stats
    m_start_time = std::chrono::high_resolution_clock::now();

    // Start video capture
    if (!m_video_capture->start()) {
        std::cerr << "Failed to start video capture\n";
        return false;
    }

    // Start audio capture if enabled
    if (m_audio_capture && !m_audio_capture->start()) {
        std::cerr << "Failed to start audio capture\n";
        m_video_capture->stop();
        return false;
    }

    // Start capture thread
    m_capture_thread = std::thread(&CaptureEngine::capture_loop, this);
    m_is_capturing = true;

    return true;
}

void CaptureEngine::stop_capture() {
    if (!m_is_capturing) {
        return;
    }

    m_should_stop = true;

    // Stop captures
    if (m_video_capture) {
        m_video_capture->stop();
    }
    if (m_audio_capture) {
        m_audio_capture->stop();
    }

    // Wait for capture thread to finish
    if (m_capture_thread.joinable()) {
        m_capture_thread.join();
    }

    // Finalize encoder and write remaining data
    if (m_encoder) {
        auto final_data = m_encoder->finalize();
        if (!final_data.empty() && m_mp4_writer) {
            // Write final frames with appropriate timestamps
            uint64_t final_timestamp = m_stats.frames_captured * 1000 / m_settings.target_fps;
            m_mp4_writer->write_video_packet(final_data, final_timestamp);
        }
    }

    // Finalize MP4 container
    if (m_mp4_writer) {
        m_mp4_writer->finalize();
    }

    // Close file writer (if still used for other purposes)
    if (m_file_writer) {
        m_file_writer->close();
    }

    m_is_capturing = false;
}

bool CaptureEngine::is_capturing() const {
    return m_is_capturing;
}

CaptureEngine::Stats CaptureEngine::get_stats() const {
    auto current_time = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(current_time - m_start_time);
    
    Stats stats = m_stats;
    if (elapsed.count() > 0) {
        stats.average_fps = static_cast<double>(stats.frames_captured) / elapsed.count();
    }
    
    if (m_file_writer) {
        stats.file_size_bytes = m_file_writer->get_file_size();
    }
    
    return stats;
}

void CaptureEngine::capture_loop() {
    auto target_frame_duration = std::chrono::microseconds(1000000 / m_settings.target_fps);
    auto last_frame_time = std::chrono::high_resolution_clock::now();

    while (!m_should_stop) {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed = current_time - last_frame_time;

        if (elapsed >= target_frame_duration) {
            // Frame timing is handled by the capture callbacks
            last_frame_time = current_time;
        }

        // Small sleep to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void CaptureEngine::process_video_frame(const Frame& frame) {
    if (!m_encoder || !m_mp4_writer) {
        return;
    }

    try {
        auto encoded_data = m_encoder->encode_video_frame(frame);
        if (!encoded_data.empty()) {
            // Calculate timestamp in milliseconds
            uint64_t timestamp_ms = m_stats.frames_captured * 1000 / m_settings.target_fps;
            
            if (m_mp4_writer->write_video_packet(encoded_data, timestamp_ms)) {
                m_stats.frames_captured++;
            } else {
                m_stats.frames_dropped++;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing video frame: " << e.what() << "\n";
        m_stats.frames_dropped++;
    }
}

void CaptureEngine::process_audio_sample(const AudioSample& sample) {
    if (!m_encoder || !m_mp4_writer) {
        return;
    }

    try {
        auto encoded_data = m_encoder->encode_audio_sample(sample);
        if (!encoded_data.empty()) {
            // Calculate timestamp based on audio sample count
            // Assuming 1024 samples per AAC frame at the sample rate
            static uint64_t audio_frame_count = 0;
            uint64_t timestamp_ms = (audio_frame_count * 1024 * 1000) / sample.sample_rate;
            
            if (m_mp4_writer->write_audio_packet(encoded_data, timestamp_ms)) {
                audio_frame_count++;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing audio sample: " << e.what() << "\n";
    }
}

} // namespace playrec