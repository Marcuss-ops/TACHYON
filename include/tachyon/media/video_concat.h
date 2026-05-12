#pragma once

#include "tachyon/core/types/media_error.h"
#include <string>
#include <vector>
#include <filesystem>

namespace tachyon::media {

/**
 * @brief Configuration for video concatenation.
 * Derived from ruststream-core/video/mod.rs
 */
struct ConcatConfig {
    std::vector<std::filesystem::path> inputs;
    std::filesystem::path output;
    std::string video_codec{"libx264"};
    std::uint32_t crf{23};
    bool use_aac{true};
};

/**
 * @brief Utility for concatenating multiple video files using FFmpeg.
 * 
 * Uses the FFmpeg concat demuxer to efficiently merge clips with 
 * minimal re-encoding where possible, or full transcoding for consistency.
 */
class VideoConcat {
public:
    /**
     * @brief Concatenates videos and returns success or failure.
     * @param config The concatenation configuration.
     * @return A MediaResult indicating success or failure.
     */
    static core::MediaResult<void> concat_videos(const ConcatConfig& config);
};

} // namespace tachyon::media
