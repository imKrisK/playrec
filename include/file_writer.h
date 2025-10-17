#pragma once

#include <string>
#include <vector>
#include <fstream>

namespace playrec {

class FileWriter {
public:
    FileWriter();
    ~FileWriter();

    // Open file for writing
    bool open(const std::string& filename);

    // Close file
    void close();

    // Write data to file
    bool write(const std::vector<uint8_t>& data);

    // Write raw data
    bool write(const uint8_t* data, size_t size);

    // Get current file size
    uint64_t get_file_size() const;

    // Check if file is open
    bool is_open() const;

    // Flush data to disk
    void flush();

private:
    std::ofstream m_file;
    uint64_t m_bytes_written = 0;
};

// MP4 container writer
class MP4Writer : public FileWriter {
public:
    MP4Writer();
    ~MP4Writer();

    // Initialize MP4 container with video/audio parameters
    bool initialize(const std::string& filename,
                   int video_width, int video_height, int fps,
                   int audio_sample_rate, int audio_channels);

    // Write video packet
    bool write_video_packet(const std::vector<uint8_t>& packet, uint64_t timestamp_ms);

    // Write audio packet  
    bool write_audio_packet(const std::vector<uint8_t>& packet, uint64_t timestamp_ms);

    // Finalize and close MP4 file
    bool finalize();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace playrec