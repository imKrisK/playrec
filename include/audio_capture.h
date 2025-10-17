#pragma once

#include "common.h"
#include <functional>
#include <thread>
#include <atomic>

namespace playrec {

class AudioCapture {
public:
    AudioCapture();
    virtual ~AudioCapture();

    // Initialize audio capture
    virtual bool initialize(const CaptureSettings& settings) = 0;

    // Start capturing audio
    virtual bool start() = 0;

    // Stop capturing
    virtual void stop() = 0;

    // Set callback for captured audio samples
    void set_sample_callback(std::function<void(const AudioSample&)> callback);

    // Get current audio format
    virtual AudioFormat get_format() const = 0;
    virtual int get_sample_rate() const = 0;
    virtual int get_channels() const = 0;

    // Check if capture is active
    virtual bool is_active() const = 0;

protected:
    void emit_sample(const AudioSample& sample);

private:
    std::function<void(const AudioSample&)> m_sample_callback;
};

// Platform-specific implementations
#ifdef _WIN32
class WindowsAudioCapture : public AudioCapture {
public:
    bool initialize(const CaptureSettings& settings) override;
    bool start() override;
    void stop() override;
    AudioFormat get_format() const override;
    int get_sample_rate() const override;
    int get_channels() const override;
    bool is_active() const override;

private:
    // Windows-specific members (WASAPI)
    bool m_is_active = false;
    AudioFormat m_format = AudioFormat::PCM_S16LE;
    int m_sample_rate = 44100;
    int m_channels = 2;
};
#endif

#ifdef __APPLE__
class MacOSAudioCapture : public AudioCapture {
public:
    bool initialize(const CaptureSettings& settings) override;
    bool start() override;
    void stop() override;
    AudioFormat get_format() const override;
    int get_sample_rate() const override;
    int get_channels() const override;
    bool is_active() const override;

private:
    void capture_loop();
    void generate_audio_sample(int sample_count);
    
    // macOS-specific members
    bool m_is_active = false;
    AudioFormat m_format = AudioFormat::PCM_S16LE;
    int m_sample_rate = 44100;
    int m_channels = 2;
    CaptureSettings m_settings;
    
    // Threading
    std::thread m_capture_thread;
    std::atomic<bool> m_should_stop{false};
};
#endif

#ifdef __linux__
class LinuxAudioCapture : public AudioCapture {
public:
    bool initialize(const CaptureSettings& settings) override;
    bool start() override;
    void stop() override;
    AudioFormat get_format() const override;
    int get_sample_rate() const override;
    int get_channels() const override;
    bool is_active() const override;

private:
    // Linux-specific members (ALSA/PulseAudio)
    bool m_is_active = false;
    AudioFormat m_format = AudioFormat::PCM_S16LE;
    int m_sample_rate = 44100;
    int m_channels = 2;
};
#endif

// Factory function
std::unique_ptr<AudioCapture> create_audio_capture();

} // namespace playrec