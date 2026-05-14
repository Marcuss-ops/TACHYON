#pragma once

#include "tachyon/core/types/media_error.h"
#include <string>
#include <vector>
#include <filesystem>

namespace tachyon::audio {

/**
 * @brief Handles audio resampling and re-encoding (WAV/AAC) using FFmpeg.
 */
class AudioResampler {
public:
    struct Config {
        std::filesystem::path input_path;
        std::filesystem::path output_path;
        int sample_rate = 44100;
        int channels = 2;
        std::string format = "wav"; // "wav" or "aac"
        int bitrate_kbps = 128;
    };

    static tachyon::core::MediaResult<std::string> resample(const Config& config);

private:
    static std::string build_ffmpeg_command(const Config& config);
};

} // namespace tachyon::audio
