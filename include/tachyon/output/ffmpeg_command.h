#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <optional>

namespace tachyon::output {

class FfmpegCommand {
public:
    FfmpegCommand& overwrite(bool enable = true);

    FfmpegCommand& input_path(const std::filesystem::path& path);
    FfmpegCommand& input_pipe();

    FfmpegCommand& raw_video_input(int width, int height, double fps, const std::string& pix_fmt = "rgba");

    FfmpegCommand& output_path(const std::filesystem::path& path);

    FfmpegCommand& video_codec(const std::string& codec);
    FfmpegCommand& video_bitrate_kbps(int bitrate_kbps);
    FfmpegCommand& video_crf(int crf);
    FfmpegCommand& video_preset(const std::string& preset);
    FfmpegCommand& video_pixel_format(const std::string& pix_fmt);
    FfmpegCommand& video_profile(const std::string& profile);

    FfmpegCommand& audio_codec(const std::string& codec);
    FfmpegCommand& audio_bitrate_kbps(int bitrate_kbps);

    FfmpegCommand& add_filter(const std::string& filter);

    FfmpegCommand& color_primaries(const std::string& primaries);
    FfmpegCommand& color_transfer(const std::string& transfer);
    FfmpegCommand& color_space(const std::string& space);
    FfmpegCommand& color_range(const std::string& range);

    FfmpegCommand& movflags_faststart(bool enable = true);

    FfmpegCommand& codec_copy();

    FfmpegCommand& extra_arg(const std::string& arg);

    std::string str() const;

private:
    std::string build_input_args() const;
    std::string build_output_args() const;

    bool m_overwrite{false};
    std::optional<std::filesystem::path> m_input_path;
    bool m_input_pipe{false};
    std::optional<std::filesystem::path> m_output_path;

    struct VideoOptions {
        std::optional<std::string> codec;
        std::optional<int> bitrate_kbps;
        std::optional<int> crf;
        std::optional<std::string> preset;
        std::optional<std::string> pixel_format;
        std::optional<std::string> profile;
    };
    VideoOptions m_video;

    struct RawVideoInput {
        bool enabled{false};
        int width{0};
        int height{0};
        double fps{0.0};
        std::string pix_fmt{"rgba"};
    };
    RawVideoInput m_raw_video;

    struct AudioOptions {
        std::optional<std::string> codec;
        std::optional<int> bitrate_kbps;
    };
    AudioOptions m_audio;

    std::vector<std::string> m_filters;
    std::optional<std::string> m_color_primaries;
    std::optional<std::string> m_color_transfer;
    std::optional<std::string> m_color_space;
    std::optional<std::string> m_color_range;

    bool m_movflags_faststart{false};
    bool m_codec_copy{false};

    std::vector<std::string> m_extra_args;
};

} // namespace tachyon::output
