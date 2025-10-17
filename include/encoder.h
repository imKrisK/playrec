#pragma once

#include "common.h"
#include <vector>

namespace playrec {

class Encoder {
public:
    Encoder();
    virtual ~Encoder();

    // Initialize encoder with settings
    virtual bool initialize(const CaptureSettings& settings, 
                          int video_width, int video_height,
                          AudioFormat audio_format, int sample_rate, int channels) = 0;

    // Encode a video frame
    virtual std::vector<uint8_t> encode_video_frame(const Frame& frame) = 0;

    // Encode an audio sample
    virtual std::vector<uint8_t> encode_audio_sample(const AudioSample& sample) = 0;

    // Finalize encoding (flush remaining data)
    virtual std::vector<uint8_t> finalize() = 0;

    // Get encoder info
    virtual std::string get_codec_name() const = 0;
    virtual bool supports_hardware_acceleration() const = 0;
};

// H.264 encoder implementation
class H264Encoder : public Encoder {
public:
    H264Encoder();
    ~H264Encoder() override;

    bool initialize(const CaptureSettings& settings,
                   int video_width, int video_height,
                   AudioFormat audio_format, int sample_rate, int channels) override;

    std::vector<uint8_t> encode_video_frame(const Frame& frame) override;
    std::vector<uint8_t> encode_audio_sample(const AudioSample& sample) override;
    std::vector<uint8_t> finalize() override;

    std::string get_codec_name() const override { return "H.264"; }
    bool supports_hardware_acceleration() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// H.265/HEVC encoder implementation
class H265Encoder : public Encoder {
public:
    H265Encoder();
    ~H265Encoder() override;

    bool initialize(const CaptureSettings& settings,
                   int video_width, int video_height,
                   AudioFormat audio_format, int sample_rate, int channels) override;

    std::vector<uint8_t> encode_video_frame(const Frame& frame) override;
    std::vector<uint8_t> encode_audio_sample(const AudioSample& sample) override;
    std::vector<uint8_t> finalize() override;

    std::string get_codec_name() const override { return "H.265/HEVC"; }
    bool supports_hardware_acceleration() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// Factory function
std::unique_ptr<Encoder> create_encoder(const std::string& codec_name = "h264");

} // namespace playrec