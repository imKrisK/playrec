#include "encoder.h"
#include <iostream>
#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
}

namespace playrec {

// Base Encoder implementation
Encoder::Encoder() = default;
Encoder::~Encoder() = default;

// H.264 Encoder implementation
struct H264Encoder::Impl {
    AVCodecContext* video_codec_context = nullptr;
    AVCodecContext* audio_codec_context = nullptr;
    AVFrame* video_frame = nullptr;
    AVFrame* audio_frame = nullptr;
    AVPacket* packet = nullptr;
    SwsContext* sws_context = nullptr;
    SwrContext* swr_context = nullptr;
    
    bool initialized = false;
    CaptureSettings settings;
    int video_width = 0;
    int video_height = 0;
    AudioFormat audio_format = AudioFormat::PCM_S16LE;
    int sample_rate = 44100;
    int channels = 2;
    int64_t video_pts = 0;
    int64_t audio_pts = 0;
    
    ~Impl() {
        cleanup();
    }
    
    void cleanup() {
        if (sws_context) {
            sws_freeContext(sws_context);
            sws_context = nullptr;
        }
        if (swr_context) {
            swr_free(&swr_context);
        }
        if (packet) {
            av_packet_free(&packet);
        }
        if (video_frame) {
            av_frame_free(&video_frame);
        }
        if (audio_frame) {
            av_frame_free(&audio_frame);
        }
        if (video_codec_context) {
            avcodec_free_context(&video_codec_context);
        }
        if (audio_codec_context) {
            avcodec_free_context(&audio_codec_context);
        }
    }
};

H264Encoder::H264Encoder() : m_impl(std::make_unique<Impl>()) {}
H264Encoder::~H264Encoder() = default;

bool H264Encoder::initialize(const CaptureSettings& settings,
                            int video_width, int video_height,
                            AudioFormat audio_format, int sample_rate, int channels) {
    
    m_impl->settings = settings;
    m_impl->video_width = video_width;
    m_impl->video_height = video_height;
    m_impl->audio_format = audio_format;
    m_impl->sample_rate = sample_rate;
    m_impl->channels = channels;
    
    // Initialize video encoder (H.264)
    const AVCodec* video_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!video_codec) {
        std::cerr << "H.264 encoder not found" << std::endl;
        return false;
    }
    
    m_impl->video_codec_context = avcodec_alloc_context3(video_codec);
    if (!m_impl->video_codec_context) {
        std::cerr << "Could not allocate video codec context" << std::endl;
        return false;
    }
    
    // Set video codec parameters using settings
    m_impl->video_codec_context->bit_rate = settings.videoBitrate;
    m_impl->video_codec_context->width = video_width;
    m_impl->video_codec_context->height = video_height;
    m_impl->video_codec_context->time_base = {1, settings.frameRate};
    m_impl->video_codec_context->framerate = {settings.frameRate, 1};
    m_impl->video_codec_context->gop_size = 10;
    m_impl->video_codec_context->max_b_frames = 1;
    m_impl->video_codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
    
    // Set H.264 specific options for better compression
    av_opt_set(m_impl->video_codec_context->priv_data, "preset", "fast", 0);
    av_opt_set(m_impl->video_codec_context->priv_data, "crf", "28", 0);  // Higher CRF = lower bitrate
    
    // Add bitrate control for consistent file sizes
    m_impl->video_codec_context->bit_rate = 8000000;  // 8 Mbps max
    m_impl->video_codec_context->rc_max_rate = 10000000;  // 10 Mbps peak
    m_impl->video_codec_context->rc_buffer_size = 16000000;  // 16 Mbps buffer
    
    // Open video codec
    if (avcodec_open2(m_impl->video_codec_context, video_codec, nullptr) < 0) {
        std::cerr << "Could not open video codec" << std::endl;
        return false;
    }
    
    // Initialize audio encoder (AAC)
    const AVCodec* audio_codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!audio_codec) {
        std::cerr << "AAC encoder not found" << std::endl;
        return false;
    }
    
    m_impl->audio_codec_context = avcodec_alloc_context3(audio_codec);
    if (!m_impl->audio_codec_context) {
        std::cerr << "Could not allocate audio codec context" << std::endl;
        return false;
    }
    
    // Set audio codec parameters
    m_impl->audio_codec_context->bit_rate = 128000; // 128 kbps
    m_impl->audio_codec_context->sample_rate = sample_rate;
    av_channel_layout_default(&m_impl->audio_codec_context->ch_layout, channels);
    m_impl->audio_codec_context->sample_fmt = AV_SAMPLE_FMT_FLTP;
    m_impl->audio_codec_context->time_base = {1, sample_rate};
    
    // Open audio codec
    if (avcodec_open2(m_impl->audio_codec_context, audio_codec, nullptr) < 0) {
        std::cerr << "Could not open audio codec" << std::endl;
        return false;
    }
    
    // Allocate frames and packet
    m_impl->video_frame = av_frame_alloc();
    m_impl->audio_frame = av_frame_alloc();
    m_impl->packet = av_packet_alloc();
    
    if (!m_impl->video_frame || !m_impl->audio_frame || !m_impl->packet) {
        std::cerr << "Could not allocate frames or packet" << std::endl;
        return false;
    }
    
    // Setup video frame
    m_impl->video_frame->format = m_impl->video_codec_context->pix_fmt;
    m_impl->video_frame->width = m_impl->video_codec_context->width;
    m_impl->video_frame->height = m_impl->video_codec_context->height;
    
    if (av_frame_get_buffer(m_impl->video_frame, 0) < 0) {
        std::cerr << "Could not allocate video frame buffer" << std::endl;
        return false;
    }
    
    // Setup audio frame
    m_impl->audio_frame->format = m_impl->audio_codec_context->sample_fmt;
    m_impl->audio_frame->nb_samples = m_impl->audio_codec_context->frame_size;
    av_channel_layout_copy(&m_impl->audio_frame->ch_layout, &m_impl->audio_codec_context->ch_layout);
    
    if (av_frame_get_buffer(m_impl->audio_frame, 0) < 0) {
        std::cerr << "Could not allocate audio frame buffer" << std::endl;
        return false;
    }
    
    // Initialize scaling context for video format conversion
    m_impl->sws_context = sws_getContext(
        video_width, video_height, AV_PIX_FMT_RGB24,
        video_width, video_height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    
    if (!m_impl->sws_context) {
        std::cerr << "Could not initialize video scaling context" << std::endl;
        return false;
    }
    
    // Initialize resampling context for audio format conversion
    m_impl->swr_context = swr_alloc();
    if (!m_impl->swr_context) {
        std::cerr << "Could not allocate audio resampling context" << std::endl;
        return false;
    }
    
    AVChannelLayout in_ch_layout;
    av_channel_layout_default(&in_ch_layout, channels);
    
    av_opt_set_chlayout(m_impl->swr_context, "in_chlayout", &in_ch_layout, 0);
    av_opt_set_chlayout(m_impl->swr_context, "out_chlayout", &m_impl->audio_codec_context->ch_layout, 0);
    av_opt_set_int(m_impl->swr_context, "in_sample_rate", sample_rate, 0);
    av_opt_set_int(m_impl->swr_context, "out_sample_rate", sample_rate, 0);
    av_opt_set_sample_fmt(m_impl->swr_context, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    av_opt_set_sample_fmt(m_impl->swr_context, "out_sample_fmt", AV_SAMPLE_FMT_FLTP, 0);
    
    if (swr_init(m_impl->swr_context) < 0) {
        std::cerr << "Could not initialize audio resampling context" << std::endl;
        return false;
    }
    
    m_impl->initialized = true;
    
    std::cout << "H.264 encoder initialized:\n";
    std::cout << "  Video: " << video_width << "x" << video_height << " @ 30fps\n";
    std::cout << "  Audio: " << sample_rate << "Hz, " << channels << " channels\n";
    
    return true;
}

std::vector<uint8_t> H264Encoder::encode_video_frame(const Frame& frame) {
    if (!m_impl->initialized) {
        return {};
    }
    
    std::vector<uint8_t> result;
    
    // Make frame writable
    if (av_frame_make_writable(m_impl->video_frame) < 0) {
        std::cerr << "Could not make video frame writable" << std::endl;
        return result;
    }
    
    // Convert RGB to YUV420P
    const uint8_t* src_data[4] = {frame.data.data(), nullptr, nullptr, nullptr};
    int src_linesize[4] = {m_impl->video_width * 3, 0, 0, 0};
    
    sws_scale(m_impl->sws_context, src_data, src_linesize, 0, m_impl->video_height,
              m_impl->video_frame->data, m_impl->video_frame->linesize);
    
    m_impl->video_frame->pts = m_impl->video_pts++;
    
    // Send frame to encoder
    int ret = avcodec_send_frame(m_impl->video_codec_context, m_impl->video_frame);
    if (ret < 0) {
        std::cerr << "Error sending video frame to encoder" << std::endl;
        return result;
    }
    
    // Receive encoded packet
    while (ret >= 0) {
        ret = avcodec_receive_packet(m_impl->video_codec_context, m_impl->packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            std::cerr << "Error encoding video frame" << std::endl;
            break;
        }
        
        // Copy packet data
        result.insert(result.end(), m_impl->packet->data, m_impl->packet->data + m_impl->packet->size);
        
        av_packet_unref(m_impl->packet);
    }
    
    return result;
}

std::vector<uint8_t> H264Encoder::encode_audio_sample(const AudioSample& sample) {
    if (!m_impl->initialized) {
        return {};
    }
    
    std::vector<uint8_t> result;
    
    // Make frame writable
    if (av_frame_make_writable(m_impl->audio_frame) < 0) {
        std::cerr << "Could not make audio frame writable" << std::endl;
        return result;
    }
    
    // Convert audio format with validation
    const uint8_t* src_data[1] = {sample.data.data()};
    int src_samples = sample.data.size() / (m_impl->channels * 2); // 16-bit samples
    
    // Validate input audio data to prevent NaN values
    if (sample.data.empty() || src_samples <= 0) {
        std::cerr << "Invalid audio sample data" << std::endl;
        return result;
    }
    
    // Clear the audio frame buffer to prevent leftover data
    if (av_frame_make_writable(m_impl->audio_frame) < 0) {
        std::cerr << "Could not make audio frame writable" << std::endl;
        return result;
    }
    
    // Zero out the audio frame data
    memset(m_impl->audio_frame->data[0], 0, 
           m_impl->audio_frame->nb_samples * av_get_bytes_per_sample(AV_SAMPLE_FMT_FLTP) * m_impl->channels);
    
    int converted_samples = swr_convert(m_impl->swr_context,
                                       m_impl->audio_frame->data, m_impl->audio_frame->nb_samples,
                                       src_data, src_samples);
    
    if (converted_samples < 0) {
        std::cerr << "Error converting audio samples" << std::endl;
        return result;
    }
    
    m_impl->audio_frame->pts = m_impl->audio_pts;
    m_impl->audio_pts += m_impl->audio_frame->nb_samples;
    
    // Send frame to encoder
    int ret = avcodec_send_frame(m_impl->audio_codec_context, m_impl->audio_frame);
    if (ret < 0) {
        std::cerr << "Error sending audio frame to encoder" << std::endl;
        return result;
    }
    
    // Receive encoded packet
    while (ret >= 0) {
        ret = avcodec_receive_packet(m_impl->audio_codec_context, m_impl->packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            std::cerr << "Error encoding audio frame" << std::endl;
            break;
        }
        
        // Copy packet data
        result.insert(result.end(), m_impl->packet->data, m_impl->packet->data + m_impl->packet->size);
        
        av_packet_unref(m_impl->packet);
    }
    
    return result;
}

std::vector<uint8_t> H264Encoder::finalize() {
    if (!m_impl->initialized) {
        return {};
    }
    
    std::vector<uint8_t> result;
    
    // Flush video encoder
    avcodec_send_frame(m_impl->video_codec_context, nullptr);
    int ret = 0;
    while (ret >= 0) {
        ret = avcodec_receive_packet(m_impl->video_codec_context, m_impl->packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        result.insert(result.end(), m_impl->packet->data, m_impl->packet->data + m_impl->packet->size);
        av_packet_unref(m_impl->packet);
    }
    
    // Flush audio encoder
    avcodec_send_frame(m_impl->audio_codec_context, nullptr);
    ret = 0;
    while (ret >= 0) {
        ret = avcodec_receive_packet(m_impl->audio_codec_context, m_impl->packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        result.insert(result.end(), m_impl->packet->data, m_impl->packet->data + m_impl->packet->size);
        av_packet_unref(m_impl->packet);
    }
    
    std::cout << "H.264 encoder finalized\n";
    return result;
}

bool H264Encoder::supports_hardware_acceleration() const {
    // Check for hardware acceleration support
    #if defined(__APPLE__)
        return avcodec_find_encoder_by_name("h264_videotoolbox") != nullptr;
    #elif defined(_WIN32)
        return avcodec_find_encoder_by_name("h264_nvenc") != nullptr ||
               avcodec_find_encoder_by_name("h264_qsv") != nullptr;
    #else
        return avcodec_find_encoder_by_name("h264_vaapi") != nullptr ||
               avcodec_find_encoder_by_name("h264_nvenc") != nullptr;
    #endif
}

// H.265 Encoder implementation
struct H265Encoder::Impl {
    AVCodecContext* video_codec_context = nullptr;
    AVCodecContext* audio_codec_context = nullptr;
    AVFrame* video_frame = nullptr;
    AVFrame* audio_frame = nullptr;
    AVPacket* packet = nullptr;
    SwsContext* sws_context = nullptr;
    SwrContext* swr_context = nullptr;
    
    bool initialized = false;
    CaptureSettings settings;
    int video_width = 0;
    int video_height = 0;
    AudioFormat audio_format = AudioFormat::PCM_S16LE;
    int sample_rate = 44100;
    int channels = 2;
    int64_t video_pts = 0;
    int64_t audio_pts = 0;
    
    ~Impl() {
        cleanup();
    }
    
    void cleanup() {
        if (sws_context) {
            sws_freeContext(sws_context);
            sws_context = nullptr;
        }
        if (swr_context) {
            swr_free(&swr_context);
        }
        if (packet) {
            av_packet_free(&packet);
        }
        if (video_frame) {
            av_frame_free(&video_frame);
        }
        if (audio_frame) {
            av_frame_free(&audio_frame);
        }
        if (video_codec_context) {
            avcodec_free_context(&video_codec_context);
        }
        if (audio_codec_context) {
            avcodec_free_context(&audio_codec_context);
        }
    }
};

H265Encoder::H265Encoder() : m_impl(std::make_unique<Impl>()) {}
H265Encoder::~H265Encoder() = default;

bool H265Encoder::initialize(const CaptureSettings& settings,
                            int video_width, int video_height,
                            AudioFormat audio_format, int sample_rate, int channels) {
    
    m_impl->settings = settings;
    m_impl->video_width = video_width;
    m_impl->video_height = video_height;
    m_impl->audio_format = audio_format;
    m_impl->sample_rate = sample_rate;
    m_impl->channels = channels;
    
    // Initialize video encoder (H.265)
    const AVCodec* video_codec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
    if (!video_codec) {
        std::cerr << "H.265 encoder not found" << std::endl;
        return false;
    }
    
    m_impl->video_codec_context = avcodec_alloc_context3(video_codec);
    if (!m_impl->video_codec_context) {
        std::cerr << "Could not allocate video codec context" << std::endl;
        return false;
    }
    
    // Set video codec parameters using settings (H.265 is more efficient)
    m_impl->video_codec_context->bit_rate = settings.videoBitrate * 0.7; // 30% less for H.265 efficiency
    m_impl->video_codec_context->width = video_width;
    m_impl->video_codec_context->height = video_height;
    m_impl->video_codec_context->time_base = {1, settings.frameRate};
    m_impl->video_codec_context->framerate = {settings.frameRate, 1};
    m_impl->video_codec_context->gop_size = 10;
    m_impl->video_codec_context->max_b_frames = 1;
    m_impl->video_codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
    
    // Set H.265 specific options
    av_opt_set(m_impl->video_codec_context->priv_data, "preset", "medium", 0);
    av_opt_set(m_impl->video_codec_context->priv_data, "crf", "25", 0);
    
    // Open video codec
    if (avcodec_open2(m_impl->video_codec_context, video_codec, nullptr) < 0) {
        std::cerr << "Could not open video codec" << std::endl;
        return false;
    }
    
    // Audio encoding is same as H.264 (AAC)
    const AVCodec* audio_codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!audio_codec) {
        std::cerr << "AAC encoder not found" << std::endl;
        return false;
    }
    
    m_impl->audio_codec_context = avcodec_alloc_context3(audio_codec);
    if (!m_impl->audio_codec_context) {
        std::cerr << "Could not allocate audio codec context" << std::endl;
        return false;
    }
    
    m_impl->audio_codec_context->bit_rate = 128000;
    m_impl->audio_codec_context->sample_rate = sample_rate;
    av_channel_layout_default(&m_impl->audio_codec_context->ch_layout, channels);
    m_impl->audio_codec_context->sample_fmt = AV_SAMPLE_FMT_FLTP;
    m_impl->audio_codec_context->time_base = {1, sample_rate};
    
    if (avcodec_open2(m_impl->audio_codec_context, audio_codec, nullptr) < 0) {
        std::cerr << "Could not open audio codec" << std::endl;
        return false;
    }
    
    // Setup frames and contexts (same as H.264)
    m_impl->video_frame = av_frame_alloc();
    m_impl->audio_frame = av_frame_alloc();
    m_impl->packet = av_packet_alloc();
    
    if (!m_impl->video_frame || !m_impl->audio_frame || !m_impl->packet) {
        std::cerr << "Could not allocate frames or packet" << std::endl;
        return false;
    }
    
    // Setup video frame
    m_impl->video_frame->format = m_impl->video_codec_context->pix_fmt;
    m_impl->video_frame->width = m_impl->video_codec_context->width;
    m_impl->video_frame->height = m_impl->video_codec_context->height;
    
    if (av_frame_get_buffer(m_impl->video_frame, 0) < 0) {
        std::cerr << "Could not allocate video frame buffer" << std::endl;
        return false;
    }
    
    // Setup audio frame
    m_impl->audio_frame->format = m_impl->audio_codec_context->sample_fmt;
    m_impl->audio_frame->nb_samples = m_impl->audio_codec_context->frame_size;
    av_channel_layout_copy(&m_impl->audio_frame->ch_layout, &m_impl->audio_codec_context->ch_layout);
    
    if (av_frame_get_buffer(m_impl->audio_frame, 0) < 0) {
        std::cerr << "Could not allocate audio frame buffer" << std::endl;
        return false;
    }
    
    // Initialize contexts
    m_impl->sws_context = sws_getContext(
        video_width, video_height, AV_PIX_FMT_RGB24,
        video_width, video_height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    
    if (!m_impl->sws_context) {
        std::cerr << "Could not initialize video scaling context" << std::endl;
        return false;
    }
    
    // Audio resampling context
    m_impl->swr_context = swr_alloc();
    if (!m_impl->swr_context) {
        std::cerr << "Could not allocate audio resampling context" << std::endl;
        return false;
    }
    
    AVChannelLayout in_ch_layout;
    av_channel_layout_default(&in_ch_layout, channels);
    
    av_opt_set_chlayout(m_impl->swr_context, "in_chlayout", &in_ch_layout, 0);
    av_opt_set_chlayout(m_impl->swr_context, "out_chlayout", &m_impl->audio_codec_context->ch_layout, 0);
    av_opt_set_int(m_impl->swr_context, "in_sample_rate", sample_rate, 0);
    av_opt_set_int(m_impl->swr_context, "out_sample_rate", sample_rate, 0);
    av_opt_set_sample_fmt(m_impl->swr_context, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    av_opt_set_sample_fmt(m_impl->swr_context, "out_sample_fmt", AV_SAMPLE_FMT_FLTP, 0);
    
    if (swr_init(m_impl->swr_context) < 0) {
        std::cerr << "Could not initialize audio resampling context" << std::endl;
        return false;
    }
    
    m_impl->initialized = true;
    
    std::cout << "H.265 encoder initialized:\n";
    std::cout << "  Video: " << video_width << "x" << video_height << " @ 30fps\n";
    std::cout << "  Audio: " << sample_rate << "Hz, " << channels << " channels\n";
    
    return true;
}

std::vector<uint8_t> H265Encoder::encode_video_frame(const Frame& frame) {
    // Same implementation as H.264 but with H.265 codec
    if (!m_impl->initialized) {
        return {};
    }
    
    std::vector<uint8_t> result;
    
    if (av_frame_make_writable(m_impl->video_frame) < 0) {
        std::cerr << "Could not make video frame writable" << std::endl;
        return result;
    }
    
    const uint8_t* src_data[4] = {frame.data.data(), nullptr, nullptr, nullptr};
    int src_linesize[4] = {m_impl->video_width * 3, 0, 0, 0};
    
    sws_scale(m_impl->sws_context, src_data, src_linesize, 0, m_impl->video_height,
              m_impl->video_frame->data, m_impl->video_frame->linesize);
    
    m_impl->video_frame->pts = m_impl->video_pts++;
    
    int ret = avcodec_send_frame(m_impl->video_codec_context, m_impl->video_frame);
    if (ret < 0) {
        std::cerr << "Error sending video frame to encoder" << std::endl;
        return result;
    }
    
    while (ret >= 0) {
        ret = avcodec_receive_packet(m_impl->video_codec_context, m_impl->packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            std::cerr << "Error encoding video frame" << std::endl;
            break;
        }
        
        result.insert(result.end(), m_impl->packet->data, m_impl->packet->data + m_impl->packet->size);
        av_packet_unref(m_impl->packet);
    }
    
    return result;
}

std::vector<uint8_t> H265Encoder::encode_audio_sample(const AudioSample& sample) {
    // Same implementation as H.264
    if (!m_impl->initialized) {
        return {};
    }
    
    std::vector<uint8_t> result;
    
    if (av_frame_make_writable(m_impl->audio_frame) < 0) {
        std::cerr << "Could not make audio frame writable" << std::endl;
        return result;
    }
    
    const uint8_t* src_data[1] = {sample.data.data()};
    int src_samples = sample.data.size() / (m_impl->channels * 2);
    
    int converted_samples = swr_convert(m_impl->swr_context,
                                       m_impl->audio_frame->data, m_impl->audio_frame->nb_samples,
                                       src_data, src_samples);
    
    if (converted_samples < 0) {
        std::cerr << "Error converting audio samples" << std::endl;
        return result;
    }
    
    m_impl->audio_frame->pts = m_impl->audio_pts;
    m_impl->audio_pts += m_impl->audio_frame->nb_samples;
    
    int ret = avcodec_send_frame(m_impl->audio_codec_context, m_impl->audio_frame);
    if (ret < 0) {
        std::cerr << "Error sending audio frame to encoder" << std::endl;
        return result;
    }
    
    while (ret >= 0) {
        ret = avcodec_receive_packet(m_impl->audio_codec_context, m_impl->packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            std::cerr << "Error encoding audio frame" << std::endl;
            break;
        }
        
        result.insert(result.end(), m_impl->packet->data, m_impl->packet->data + m_impl->packet->size);
        av_packet_unref(m_impl->packet);
    }
    
    return result;
}

std::vector<uint8_t> H265Encoder::finalize() {
    if (!m_impl->initialized) {
        return {};
    }
    
    std::vector<uint8_t> result;
    
    // Flush encoders (same as H.264)
    avcodec_send_frame(m_impl->video_codec_context, nullptr);
    int ret = 0;
    while (ret >= 0) {
        ret = avcodec_receive_packet(m_impl->video_codec_context, m_impl->packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        result.insert(result.end(), m_impl->packet->data, m_impl->packet->data + m_impl->packet->size);
        av_packet_unref(m_impl->packet);
    }
    
    avcodec_send_frame(m_impl->audio_codec_context, nullptr);
    ret = 0;
    while (ret >= 0) {
        ret = avcodec_receive_packet(m_impl->audio_codec_context, m_impl->packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        result.insert(result.end(), m_impl->packet->data, m_impl->packet->data + m_impl->packet->size);
        av_packet_unref(m_impl->packet);
    }
    
    std::cout << "H.265 encoder finalized\n";
    return result;
}

bool H265Encoder::supports_hardware_acceleration() const {
    #if defined(__APPLE__)
        return avcodec_find_encoder_by_name("hevc_videotoolbox") != nullptr;
    #elif defined(_WIN32)
        return avcodec_find_encoder_by_name("hevc_nvenc") != nullptr ||
               avcodec_find_encoder_by_name("hevc_qsv") != nullptr;
    #else
        return avcodec_find_encoder_by_name("hevc_vaapi") != nullptr ||
               avcodec_find_encoder_by_name("hevc_nvenc") != nullptr;
    #endif
}

// Factory function
std::unique_ptr<Encoder> create_encoder(const std::string& codec_name) {
    if (codec_name == "h264" || codec_name == "H.264") {
        return std::make_unique<H264Encoder>();
    } else if (codec_name == "h265" || codec_name == "H.265" || codec_name == "hevc") {
        return std::make_unique<H265Encoder>();
    } else {
        std::cerr << "Unknown codec: " << codec_name << ". Defaulting to H.264\n";
        return std::make_unique<H264Encoder>();
    }
}

} // namespace playrec