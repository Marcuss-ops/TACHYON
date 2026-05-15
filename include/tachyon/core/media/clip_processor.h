#pragma once

#include "tachyon/core/types/media_error.h"
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
 * @brief Processes individual video clips (transcoding, scaling, fades).
 */
class ClipProcessor {
public:
    static MediaResult<void> process_clip(const ClipProcessingConfig& config);

private:
    static std::string build_filter_complex(const ClipProcessingConfig& config, double duration);
};

} // namespace tachyon::core::media
