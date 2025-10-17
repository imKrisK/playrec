#include "file_writer.h"
#include <iostream>
#include <cstring>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
#include <libavutil/error.h>
}

namespace playrec {

// Helper function to convert FFmpeg error codes to strings
static std::string av_error_to_string(int errnum) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
    return std::string(errbuf);
}

// Base FileWriter implementation
FileWriter::FileWriter() = default;

FileWriter::~FileWriter() {
    if (is_open()) {
        close();
    }
}

bool FileWriter::open(const std::string& filename) {
    if (is_open()) {
        close();
    }
    
    m_file.open(filename, std::ios::binary | std::ios::out);
    if (!m_file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << "\n";
        return false;
    }
    
    m_bytes_written = 0;
    std::cout << "File opened for writing: " << filename << "\n";
    return true;
}

void FileWriter::close() {
    if (m_file.is_open()) {
        m_file.close();
        std::cout << "File closed. Total bytes written: " << m_bytes_written << "\n";
    }
}

bool FileWriter::write(const std::vector<uint8_t>& data) {
    return write(data.data(), data.size());
}

bool FileWriter::write(const uint8_t* data, size_t size) {
    if (!is_open() || !data || size == 0) {
        return false;
    }
    
    m_file.write(reinterpret_cast<const char*>(data), size);
    if (m_file.good()) {
        m_bytes_written += size;
        return true;
    }
    
    std::cerr << "Error writing to file\n";
    return false;
}

uint64_t FileWriter::get_file_size() const {
    return m_bytes_written;
}

bool FileWriter::is_open() const {
    return m_file.is_open();
}

void FileWriter::flush() {
    if (is_open()) {
        m_file.flush();
    }
}

// MP4Writer implementation with FFmpeg libavformat
struct MP4Writer::Impl {
    AVFormatContext* format_context = nullptr;
    AVStream* video_stream = nullptr;
    AVStream* audio_stream = nullptr;
    
    std::string filename;
    bool initialized = false;
    bool finalized = false;
    bool header_written = false;
    
    // Video parameters
    int video_width = 0;
    int video_height = 0;
    int fps = 30;
    AVRational video_time_base;
    
    // Audio parameters
    int audio_sample_rate = 44100;
    int audio_channels = 2;
    AVRational audio_time_base;
    
    // Timing
    uint64_t video_frame_count = 0;
    uint64_t audio_sample_count = 0;
    int64_t last_video_pts = 0;
    int64_t last_audio_pts = 0;
    
    ~Impl() {
        cleanup();
    }
    
    void cleanup() {
        if (format_context) {
            if (header_written && !finalized) {
                av_write_trailer(format_context);
            }
            if (!(format_context->oformat->flags & AVFMT_NOFILE)) {
                avio_closep(&format_context->pb);
            }
            avformat_free_context(format_context);
            format_context = nullptr;
        }
        initialized = false;
        header_written = false;
        finalized = false;
    }
};

MP4Writer::MP4Writer() : m_impl(std::make_unique<Impl>()) {}
MP4Writer::~MP4Writer() {
    if (m_impl && m_impl->initialized && !m_impl->finalized) {
        finalize();
    }
}

bool MP4Writer::initialize(const std::string& filename,
                          int video_width, int video_height, int fps,
                          int audio_sample_rate, int audio_channels) {
    if (m_impl->initialized) {
        std::cerr << "MP4Writer already initialized\n";
        return false;
    }
    
    // Store parameters
    m_impl->filename = filename;
    m_impl->video_width = video_width;
    m_impl->video_height = video_height;
    m_impl->fps = fps;
    m_impl->audio_sample_rate = audio_sample_rate;
    m_impl->audio_channels = audio_channels;
    
    // Create output format context
    int ret = avformat_alloc_output_context2(&m_impl->format_context, nullptr, nullptr, filename.c_str());
    if (ret < 0) {
        std::cerr << "Failed to allocate output context: " << av_error_to_string(ret) << "\n";
        return false;
    }
    
    // Set time bases
    m_impl->video_time_base = {1, fps};
    m_impl->audio_time_base = {1, audio_sample_rate};
    
    // Add video stream
    m_impl->video_stream = avformat_new_stream(m_impl->format_context, nullptr);
    if (!m_impl->video_stream) {
        std::cerr << "Failed to create video stream\n";
        m_impl->cleanup();
        return false;
    }
    
    m_impl->video_stream->id = 0;
    m_impl->video_stream->time_base = m_impl->video_time_base;
    
    // Configure video stream parameters
    AVCodecParameters* video_params = m_impl->video_stream->codecpar;
    video_params->codec_type = AVMEDIA_TYPE_VIDEO;
    video_params->codec_id = AV_CODEC_ID_H264;
    video_params->width = video_width;
    video_params->height = video_height;
    video_params->format = AV_PIX_FMT_YUV420P;
    video_params->bit_rate = video_width * video_height * fps / 4; // Rough estimate
    
    // Add audio stream
    m_impl->audio_stream = avformat_new_stream(m_impl->format_context, nullptr);
    if (!m_impl->audio_stream) {
        std::cerr << "Failed to create audio stream\n";
        m_impl->cleanup();
        return false;
    }
    
    m_impl->audio_stream->id = 1;
    m_impl->audio_stream->time_base = m_impl->audio_time_base;
    
    // Configure audio stream parameters
    AVCodecParameters* audio_params = m_impl->audio_stream->codecpar;
    audio_params->codec_type = AVMEDIA_TYPE_AUDIO;
    audio_params->codec_id = AV_CODEC_ID_AAC;
    audio_params->sample_rate = audio_sample_rate;
    audio_params->ch_layout.nb_channels = audio_channels;
    av_channel_layout_default(&audio_params->ch_layout, audio_channels);
    audio_params->format = AV_SAMPLE_FMT_FLTP;
    audio_params->bit_rate = 128000; // 128 kbps
    audio_params->frame_size = 1024; // AAC frame size
    audio_params->block_align = 0; // Let FFmpeg calculate
    
    // Open output file
    if (!(m_impl->format_context->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&m_impl->format_context->pb, filename.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            std::cerr << "Failed to open output file: " << av_error_to_string(ret) << "\n";
            m_impl->cleanup();
            return false;
        }
    }
    
    m_impl->initialized = true;
    
    std::cout << "MP4 writer initialized:\n";
    std::cout << "  File: " << filename << "\n";
    std::cout << "  Video: " << video_width << "x" << video_height << " @ " << fps << " FPS\n";
    std::cout << "  Audio: " << audio_sample_rate << "Hz, " << audio_channels << " channels\n";
    std::cout << "  Video time base: " << m_impl->video_time_base.num << "/" << m_impl->video_time_base.den << "\n";
    std::cout << "  Audio time base: " << m_impl->audio_time_base.num << "/" << m_impl->audio_time_base.den << "\n";
    
    return true;
}

bool MP4Writer::write_video_packet(const std::vector<uint8_t>& packet, uint64_t timestamp_ms) {
    if (!m_impl->initialized || m_impl->finalized || packet.empty()) {
        return false;
    }
    
    // Write header on first packet
    if (!m_impl->header_written) {
        int ret = avformat_write_header(m_impl->format_context, nullptr);
        if (ret < 0) {
            std::cerr << "Failed to write header: " << av_error_to_string(ret) << "\n";
            return false;
        }
        m_impl->header_written = true;
        std::cout << "MP4 header written successfully\n";
    }
    
    // Create packet
    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        std::cerr << "Failed to allocate packet\n";
        return false;
    }
    
    // Copy packet data safely
    pkt->data = static_cast<uint8_t*>(av_malloc(packet.size()));
    if (!pkt->data) {
        std::cerr << "Failed to allocate packet data\n";
        av_packet_free(&pkt);
        return false;
    }
    memcpy(pkt->data, packet.data(), packet.size());
    pkt->size = packet.size();
    
    // Set packet properties
    pkt->stream_index = m_impl->video_stream->index;
    
    // Convert timestamp from milliseconds to stream time base
    int64_t pts = av_rescale_q(timestamp_ms, {1, 1000}, m_impl->video_stream->time_base);
    pkt->pts = pts;
    pkt->dts = pts;
    
    // Set duration (time between frames)
    pkt->duration = av_rescale_q(1, {1, m_impl->fps}, m_impl->video_stream->time_base);
    
    // Key frame detection (simplified - assume first frame and every 30th frame is keyframe)
    if (m_impl->video_frame_count == 0 || m_impl->video_frame_count % 30 == 0) {
        pkt->flags |= AV_PKT_FLAG_KEY;
    }
    
    // Write packet
    int ret = av_interleaved_write_frame(m_impl->format_context, pkt);
    if (ret < 0) {
        std::cerr << "Failed to write video packet: " << av_error_to_string(ret) << "\n";
        av_packet_free(&pkt);
        return false;
    }
    
    m_impl->video_frame_count++;
    m_impl->last_video_pts = pts;
    
    // Log progress every 100 frames
    if (m_impl->video_frame_count % 100 == 0) {
        std::cout << "Wrote video frame " << m_impl->video_frame_count 
                  << " (PTS: " << pts << ", timestamp: " << timestamp_ms << "ms)\n";
    }
    
    av_packet_free(&pkt);
    return true;
}

bool MP4Writer::write_audio_packet(const std::vector<uint8_t>& packet, uint64_t timestamp_ms) {
    if (!m_impl->initialized || m_impl->finalized || packet.empty()) {
        return false;
    }
    
    // Write header on first packet if not already written
    if (!m_impl->header_written) {
        int ret = avformat_write_header(m_impl->format_context, nullptr);
        if (ret < 0) {
            std::cerr << "Failed to write header: " << av_error_to_string(ret) << "\n";
            return false;
        }
        m_impl->header_written = true;
        std::cout << "MP4 header written successfully\n";
    }
    
    // Create packet
    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        std::cerr << "Failed to allocate audio packet\n";
        return false;
    }
    
    // Copy packet data safely
    pkt->data = static_cast<uint8_t*>(av_malloc(packet.size()));
    if (!pkt->data) {
        std::cerr << "Failed to allocate audio packet data\n";
        av_packet_free(&pkt);
        return false;
    }
    memcpy(pkt->data, packet.data(), packet.size());
    pkt->size = packet.size();
    
    // Set packet properties
    pkt->stream_index = m_impl->audio_stream->index;
    
    // Convert timestamp from milliseconds to stream time base
    int64_t pts = av_rescale_q(timestamp_ms, {1, 1000}, m_impl->audio_stream->time_base);
    pkt->pts = pts;
    pkt->dts = pts;
    
    // Set duration (AAC frame is typically 1024 samples)
    int samples_per_frame = 1024;
    pkt->duration = av_rescale_q(samples_per_frame, {1, m_impl->audio_sample_rate}, m_impl->audio_stream->time_base);
    
    // Write packet
    int ret = av_interleaved_write_frame(m_impl->format_context, pkt);
    if (ret < 0) {
        std::cerr << "Failed to write audio packet: " << av_error_to_string(ret) << "\n";
        av_packet_free(&pkt);
        return false;
    }
    
    m_impl->audio_sample_count++;
    m_impl->last_audio_pts = pts;
    
    // Log progress every 100 audio frames
    if (m_impl->audio_sample_count % 100 == 0) {
        std::cout << "Wrote audio frame " << m_impl->audio_sample_count 
                  << " (PTS: " << pts << ", timestamp: " << timestamp_ms << "ms)\n";
    }
    
    av_packet_free(&pkt);
    return true;
}

bool MP4Writer::finalize() {
    if (!m_impl->initialized || m_impl->finalized) {
        return false;
    }
    
    // Write trailer to finalize the MP4 file
    if (m_impl->header_written) {
        int ret = av_write_trailer(m_impl->format_context);
        if (ret < 0) {
            std::cerr << "Failed to write trailer: " << av_error_to_string(ret) << "\n";
            return false;
        }
    }
    
    // Calculate durations
    double video_duration = m_impl->video_frame_count / static_cast<double>(m_impl->fps);
    double audio_duration = (m_impl->audio_sample_count * 1024) / static_cast<double>(m_impl->audio_sample_rate);
    
    m_impl->finalized = true;
    
    std::cout << "MP4 writer finalized successfully:\n";
    std::cout << "  File: " << m_impl->filename << "\n";
    std::cout << "  Video frames: " << m_impl->video_frame_count << " (" << video_duration << "s)\n";
    std::cout << "  Audio frames: " << m_impl->audio_sample_count << " (" << audio_duration << "s)\n";
    std::cout << "  Final video PTS: " << m_impl->last_video_pts << "\n";
    std::cout << "  Final audio PTS: " << m_impl->last_audio_pts << "\n";
    
    return true;
}

} // namespace playrec