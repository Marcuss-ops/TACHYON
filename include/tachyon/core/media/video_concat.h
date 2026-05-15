#pragma once

#include "tachyon/core/media/media_error.h"
#include <string>
#include <vector>
#include <filesystem>

namespace tachyon::core::media {

/**
 * @brief Configuration for video concatenation.
 */
struct ConcatConfig {
    std::vector<std::filesystem::path> inputs;
    std::filesystem::path output;
    std::string video_codec{"libx264"};
    std::uint32_t crf{23};
    bool use_aac{true};
};

/**
 * @brief Contract for concatenating multiple video files.
 */
class IVideoConcat {
public:
    virtual ~IVideoConcat() = default;

    /**
     * @brief Concatenates videos and returns success or failure.
     */
    virtual MediaResult<void> concat_videos(const ConcatConfig& config) = 0;
};

} // namespace tachyon::core::media
