#pragma once

#include "tachyon/core/types/media_error.h"
#include <string>
#include <vector>
#include <filesystem>
#include <optional>
#include <utility>

namespace tachyon::media {

/**
 * @brief Specification for an individual overlay layer.
 */
struct OverlaySpec {
    std::filesystem::path path;
    double start_time{0.0};     // Start on timeline in seconds
    double duration{0.0};       // Duration in seconds
    double x{0.0};              // X position (absolute pixels)
    double y{0.0};              // Y position (absolute pixels)
    double opacity{1.0};        // Layer opacity [0, 1]
};

/**
 * @brief Configuration for the final overlay merging stage.
 * Derived from ruststream-core/video/overlay_merge.rs
 */
struct OverlayMergeConfig {
    std::filesystem::path base_video_path;
    std::vector<OverlaySpec> overlays;
    std::filesystem::path output_path;
    
    // Windows where overlays should be hidden (e.g., during central clip)
    std::vector<std::pair<double, double>> middle_clip_windows;
    
    std::optional<std::filesystem::path> subtitles_ass_path;
    std::optional<std::filesystem::path> fonts_dir;
    
    uint32_t width{1920};
    uint32_t height{1080};
    double fps{30.0};
    std::uint32_t crf{23};
};

/**
 * @brief Utility for merging multiple overlays onto a base video using FFmpeg.
 */
class OverlayMerger {
public:
    /**
     * @brief Executes the overlay merging process.
     * @param config The merge configuration.
     * @return A MediaResult indicating success or failure.
     */
    static core::MediaResult<void> merge_overlays(const OverlayMergeConfig& config);

private:
    /**
     * @brief Constructs the complex filtergraph string for FFmpeg.
     */
    static std::string build_filter_complex(const OverlayMergeConfig& config);
    
    /**
     * @brief Builds the 'enable' condition string for an overlay.
     */
    static std::string build_enable_condition(
        const OverlaySpec& overlay, 
        const std::vector<std::pair<double, double>>& middle_clips);
};

} // namespace tachyon::media
