#pragma once

#include "tachyon/core/types/media_error.h"
#include <filesystem>
#include <string>
#include <vector>
#include <optional>

namespace tachyon::core::media {

struct OverlaySpec {
    std::filesystem::path path;
    double start_time{0.0};
    double duration{0.0};
    int x{0};
    int y{0};
    double opacity{1.0};
};

struct OverlayMergeConfig {
    std::filesystem::path base_video_path;
    std::filesystem::path output_path;
    
    int width{1920};
    int height{1080};
    double fps{30.0};
    int crf{23};
    
    std::vector<OverlaySpec> overlays;
    std::vector<std::pair<double, double>> middle_clip_windows;
    
    std::optional<std::filesystem::path> subtitles_ass_path;
    std::optional<std::filesystem::path> fonts_dir;
};

/**
 * @brief Utility for merging overlays and subtitles into a base video.
 */
class OverlayMerger {
public:
    static MediaResult<void> merge_overlays(const OverlayMergeConfig& config);

private:
    static std::string build_filter_complex(const OverlayMergeConfig& config);
    static std::string build_enable_condition(const OverlaySpec& overlay, const std::vector<std::pair<double, double>>& middle_clips);
};

} // namespace tachyon::core::media
