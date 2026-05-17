#pragma once

#include <string>
#include <vector>

namespace tachyon::output {

enum class EncoderBackend {
    Software,
    Nvenc,
    Vaapi,
    VideoToolbox,
    Amf
};

struct EncoderCapability {
    std::string name;
    std::string description;
    EncoderBackend backend;
    bool supports_h264{false};
    bool supports_hevc{false};
};

struct FFmpegCapabilities {
    bool has_nvenc{false};
    bool has_vaapi{false};
    bool has_videotoolbox{false};
    bool has_amf{false};
    
    std::vector<EncoderCapability> encoders;
};

/**
 * @brief Queries FFmpeg for available encoders and parses the output.
 */
FFmpegCapabilities detect_ffmpeg_capabilities();

/**
 * @brief Internal parser for FFmpeg -encoders output.
 */
FFmpegCapabilities parse_ffmpeg_encoders(const std::string& output);

/**
 * @brief Chooses the best encoder based on capabilities and requested backend.
 */
std::string choose_ffmpeg_encoder(const std::string& requested_backend, const FFmpegCapabilities& caps);

} // namespace tachyon::output
