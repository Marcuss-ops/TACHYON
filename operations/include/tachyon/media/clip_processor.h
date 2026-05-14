#pragma once

#include "tachyon/core/types/media_error.h"
#include <string>
#include <vector>
#include <filesystem>
#include <optional>

namespace tachyon::media {

/**
 * @brief Specification for a sound effect to be mixed into a clip.
 */
struct SoundEffectSpec {
    std::filesystem::path path;
    double start_time{0.0};     // Start relative to clip start
    double volume{1.0};         // Linear volume multiplier
};

/**
 * @brief Configuration for processing a single video clip.
 * Derived from ruststream-core/video/clip_processing.rs
 */
struct ClipProcessingConfig {
    std::filesystem::path input_path;
    std::filesystem::path output_path;
    
    uint32_t width{1920};
    uint32_t height{1080};
    double fps{30.0};
    
    std::vector<SoundEffectSpec> sfx;
    std::optional<std::filesystem::path> subtitles_srt;
    std::optional<std::filesystem::path> font_path;
    
    double fade_in_seconds{0.0};
    double fade_out_seconds{0.0};
    std::uint32_t crf{23};
};

/**
 * @brief Utility for processing and preparing individual video clips using FFmpeg.
 */
class ClipProcessor {
public:
    /**
     * @brief Processes a clip and returns success or failure.
     * @param config The clip processing configuration.
     * @return A MediaResult indicating success or failure.
     */
    static core::MediaResult<void> process_clip(const ClipProcessingConfig& config);

private:
    /**
     * @brief Constructs the complex filtergraph string for FFmpeg.
     */
    static std::string build_filter_complex(const ClipProcessingConfig& config, double duration);
};

} // namespace tachyon::media
