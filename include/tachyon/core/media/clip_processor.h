#pragma once

#include "tachyon/core/media/media_error.h"
#include <filesystem>
#include <string>
#include <vector>
#include <optional>

namespace tachyon::core::media {

struct SFXSpec {
    std::filesystem::path path;
    double start_time{0.0};
    double volume{1.0};
};

struct ClipProcessingConfig {
    std::filesystem::path input_path;
    std::filesystem::path output_path;
    
    int width{1920};
    int height{1080};
    double fps{30.0};
    int crf{23};
    
    double fade_in_seconds{0.0};
    double fade_out_seconds{0.0};
    
    std::optional<std::filesystem::path> subtitles_srt;
    std::optional<std::filesystem::path> font_path;
    
    std::vector<SFXSpec> sfx;
};

/**
 * @brief Contract for processing individual video clips (transcoding, scaling, fades).
 */
class IClipProcessor {
public:
    virtual ~IClipProcessor() = default;

    /**
     * @brief Processes an individual video clip.
     * @return MediaResult<void>
     */
    virtual MediaResult<void> process_clip(const ClipProcessingConfig& config) = 0;
};

} // namespace tachyon::core::media
