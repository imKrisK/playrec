#include "video_capture.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

namespace playrec {

// Base VideoCapture implementation
VideoCapture::VideoCapture() = default;
VideoCapture::~VideoCapture() = default;

void VideoCapture::set_frame_callback(std::function<void(const Frame&)> callback) {
    m_frame_callback = std::move(callback);
}

void VideoCapture::emit_frame(const Frame& frame) {
    if (m_frame_callback) {
        m_frame_callback(frame);
    }
}

// Platform-specific implementations

#ifdef _WIN32
// Windows implementation using DXGI Desktop Duplication
bool WindowsVideoCapture::initialize(const CaptureSettings& settings) {
    // TODO: Implement Windows screen capture using DXGI Desktop Duplication API
    // This would involve:
    // 1. Initialize D3D11 device and context
    // 2. Get DXGI adapter and output
    // 3. Create desktop duplication interface
    // 4. Set up texture for frame capture
    
    std::cout << "Windows video capture initialized (placeholder)\n";
    m_width = 1920;  // Default resolution - should be detected
    m_height = 1080;
    return true;
}

bool WindowsVideoCapture::start() {
    // TODO: Start capture thread that:
    // 1. Calls AcquireNextFrame() on desktop duplication
    // 2. Maps the acquired texture
    // 3. Converts to desired format
    // 4. Calls emit_frame() with the converted frame
    
    m_is_active = true;
    std::cout << "Windows video capture started (placeholder)\n";
    return true;
}

void WindowsVideoCapture::stop() {
    m_is_active = false;
    std::cout << "Windows video capture stopped\n";
}

std::pair<int, int> WindowsVideoCapture::get_resolution() const {
    return {m_width, m_height};
}

bool WindowsVideoCapture::is_active() const {
    return m_is_active;
}
#endif

#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>

// macOS implementation using CoreGraphics
bool MacOSVideoCapture::initialize(const CaptureSettings& settings) {
    // Get main display ID
    CGDirectDisplayID displayID = CGMainDisplayID();
    
    // Get display bounds
    CGRect displayBounds = CGDisplayBounds(displayID);
    m_width = static_cast<int>(displayBounds.size.width);
    m_height = static_cast<int>(displayBounds.size.height);
    
    m_settings = settings;
    m_display_id = displayID;
    
    std::cout << "macOS video capture initialized:\n";
    std::cout << "  Resolution: " << m_width << "x" << m_height << "\n";
    std::cout << "  Display ID: " << displayID << "\n";
    
    return true;
}

bool MacOSVideoCapture::start() {
    if (m_is_active) {
        return false;
    }
    
    m_should_stop = false;
    m_is_active = true;
    
    // Start capture thread
    m_capture_thread = std::thread(&MacOSVideoCapture::capture_loop, this);
    
    std::cout << "macOS video capture started\n";
    return true;
}

void MacOSVideoCapture::stop() {
    if (!m_is_active) {
        return;
    }
    
    m_should_stop = true;
    
    if (m_capture_thread.joinable()) {
        m_capture_thread.join();
    }
    
    m_is_active = false;
    std::cout << "macOS video capture stopped\n";
}

std::pair<int, int> MacOSVideoCapture::get_resolution() const {
    return {m_width, m_height};
}

bool MacOSVideoCapture::is_active() const {
    return m_is_active;
}

void MacOSVideoCapture::capture_loop() {
    auto target_interval = std::chrono::microseconds(1000000 / m_settings.target_fps);
    auto last_capture_time = std::chrono::high_resolution_clock::now();
    
    while (!m_should_stop) {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed = current_time - last_capture_time;
        
        if (elapsed >= target_interval) {
            capture_frame();
            last_capture_time = current_time;
        }
        
        // Small sleep to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void MacOSVideoCapture::capture_frame() {
    // Note: CGDisplayCreateImage is obsoleted in macOS 15.0+
    // For now, we'll create a test pattern frame as a demonstration
    // In production, this should use ScreenCaptureKit for macOS 15.0+
    
    Frame frame;
    frame.width = m_width;
    frame.height = m_height;
    frame.format = VideoFormat::BGRA32;
    frame.timestamp = std::chrono::high_resolution_clock::now();
    
    // Create a test pattern (moving gradient)
    size_t pixel_count = m_width * m_height;
    frame.data.resize(pixel_count * 4); // 4 bytes per pixel (BGRA)
    
    // Use timestamp to create animation
    auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        frame.timestamp.time_since_epoch()).count();
    
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            size_t pixel_index = (y * m_width + x) * 4;
            
            // Create animated test pattern
            uint8_t r = static_cast<uint8_t>((x + time_ms / 10) % 256);
            uint8_t g = static_cast<uint8_t>((y + time_ms / 15) % 256);
            uint8_t b = static_cast<uint8_t>((x + y + time_ms / 20) % 256);
            
            frame.data[pixel_index + 0] = b; // Blue
            frame.data[pixel_index + 1] = g; // Green
            frame.data[pixel_index + 2] = r; // Red
            frame.data[pixel_index + 3] = 255; // Alpha
        }
    }
    
    // Emit frame to callback
    emit_frame(frame);
}
#endif

#ifdef __linux__
// Linux implementation using X11
bool LinuxVideoCapture::initialize(const CaptureSettings& settings) {
    // TODO: Implement Linux screen capture using:
    // 1. XGetImage to capture screen content
    // 2. Or use Xdamage for efficient updates
    // 3. Convert X11 image format to desired format
    
    std::cout << "Linux video capture initialized (placeholder)\n";
    m_width = 1920;  // Should detect from X11
    m_height = 1080;
    return true;
}

bool LinuxVideoCapture::start() {
    // TODO: Start X11 capture thread
    // Regularly call XGetImage and convert to frames
    
    m_is_active = true;
    std::cout << "Linux video capture started (placeholder)\n";
    return true;
}

void LinuxVideoCapture::stop() {
    m_is_active = false;
    std::cout << "Linux video capture stopped\n";
}

std::pair<int, int> LinuxVideoCapture::get_resolution() const {
    return {m_width, m_height};
}

bool LinuxVideoCapture::is_active() const {
    return m_is_active;
}
#endif

// Factory function
std::unique_ptr<VideoCapture> create_video_capture() {
#ifdef _WIN32
    return std::make_unique<WindowsVideoCapture>();
#elif defined(__APPLE__)
    return std::make_unique<MacOSVideoCapture>();
#elif defined(__linux__)
    return std::make_unique<LinuxVideoCapture>();
#else
    std::cerr << "Unsupported platform for video capture\n";
    return nullptr;
#endif
}

} // namespace playrec