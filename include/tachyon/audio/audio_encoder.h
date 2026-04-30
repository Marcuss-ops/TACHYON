#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

// Forward declarations for FFmpeg
struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVFrame;
struct AVPacket;
struct SwrContext;

namespace tachyon {
namespace audio {

struct AudioExportConfig {
    std::string codec{"aac"};
    int sample_rate{48000};
    int bitrate_kbps{192};
    int channels{2};
};

class AudioEncoder {
public:
    AudioEncoder();
    ~AudioEncoder();

    bool open(const std::filesystem::path& output_path, const AudioExportConfig& config);
    void close();

    bool encode_interleaved_float(const std::vector<float>& interleaved_samples);
    bool flush();

    [[nodiscard]] bool is_open() const noexcept { return m_format_context != nullptr; }
    [[nodiscard]] int sample_rate() const noexcept { return m_sample_rate; }
    [[nodiscard]] int channels() const noexcept { return m_channels; }

private:
    std::filesystem::path m_output_path;
    AudioExportConfig m_config;
    int m_sample_rate{48000};
    int m_channels{2};

    AVFormatContext* m_format_context{nullptr};
    AVCodecContext* m_codec_context{nullptr};
    AVStream* m_stream{nullptr};
    AVFrame* m_frame{nullptr};
    SwrContext* m_swr_context{nullptr};
    AVPacket* m_packet{nullptr};
    int m_frame_size{0};

    int64_t m_pts{0};
    bool m_opened{false};

    static constexpr int kTargetSampleRate = 48000;
    static constexpr int kTargetChannels = 2;
};

} // namespace audio
} // namespace tachyon
