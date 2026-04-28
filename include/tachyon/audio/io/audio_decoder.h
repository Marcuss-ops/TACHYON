#pragma once

#include <filesystem>
#include <vector>
#include <mutex>
#include <memory>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwrContext;

namespace tachyon::audio {

class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();

    // Disable copy
    AudioDecoder(const AudioDecoder&) = delete;
    AudioDecoder& operator=(const AudioDecoder&) = delete;

    bool open(const std::filesystem::path& path);
    void close();

    // Decodes a range of samples. Returns interleaved stereo float PCM at 48kHz.
    // If start_sec is outside duration, returns empty.
    std::vector<float> decode_range(double start_sec, double duration_sec);

    [[nodiscard]] double duration() const noexcept { return m_duration_seconds; }
    [[nodiscard]] bool is_open() const noexcept { return m_format_context != nullptr; }
    [[nodiscard]] int channels() const noexcept { return m_channels; }
    [[nodiscard]] int sample_rate() const noexcept { return m_sample_rate; }

private:
    std::filesystem::path m_path;
    double m_duration_seconds{0.0};
    int m_channels{0};
    int m_sample_rate{0};
    
    mutable std::mutex m_mutex;
    AVFormatContext* m_format_context{nullptr};
    AVCodecContext* m_codec_context{nullptr};
    AVFrame* m_frame{nullptr};
    AVPacket* m_packet{nullptr};
    SwrContext* m_swr_context{nullptr};
    int m_stream_index{-1};
    double m_stream_time_base{0.0};
    
    // Internal target for mixing
    static constexpr int kTargetSampleRate = 48000;
    static constexpr int kTargetChannels = 2;
};

} // namespace tachyon::audio
