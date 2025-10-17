#pragma once

#include "common.h"
#include <functional>
#include <thread>
#include <atomic>

namespace playrec {

class VideoCapture {
public:
    VideoCapture();
    virtual ~VideoCapture();

    // Initialize video capture
    virtual bool initialize(const CaptureSettings& settings) = 0;

    // Start capturing frames
    virtual bool start() = 0;

    // Stop capturing
    virtual void stop() = 0;

    // Set callback for captured frames
    void set_frame_callback(std::function<void(const Frame&)> callback);

    // Get current capture resolution
    virtual std::pair<int, int> get_resolution() const = 0;

    // Check if capture is active
    virtual bool is_active() const = 0;

protected:
    void emit_frame(const Frame& frame);

private:
    std::function<void(const Frame&)> m_frame_callback;
};

// Platform-specific implementations
#ifdef _WIN32
class WindowsVideoCapture : public VideoCapture {
public:
    bool initialize(const CaptureSettings& settings) override;
    bool start() override;
    void stop() override;
    std::pair<int, int> get_resolution() const override;
    bool is_active() const override;

private:
    // Windows-specific members (D3D11, DXGI)
    bool m_is_active = false;
    int m_width = 0, m_height = 0;
};
#endif

#ifdef __APPLE__
class MacOSVideoCapture : public VideoCapture {
public:
    bool initialize(const CaptureSettings& settings) override;
    bool start() override;
    void stop() override;
    std::pair<int, int> get_resolution() const override;
    bool is_active() const override;

private:
    void capture_loop();
    void capture_frame();
    
    // macOS-specific members
    bool m_is_active = false;
    int m_width = 0, m_height = 0;
    CaptureSettings m_settings;
    uint32_t m_display_id = 0;
    
    // Threading
    std::thread m_capture_thread;
    std::atomic<bool> m_should_stop{false};
};
#endif

#ifdef __linux__
class LinuxVideoCapture : public VideoCapture {
public:
    bool initialize(const CaptureSettings& settings) override;
    bool start() override;
    void stop() override;
    std::pair<int, int> get_resolution() const override;
    bool is_active() const override;

private:
    // Linux-specific members (X11, V4L2)
    bool m_is_active = false;
    int m_width = 0, m_height = 0;
};
#endif

// Factory function
std::unique_ptr<VideoCapture> create_video_capture();

} // namespace playrec