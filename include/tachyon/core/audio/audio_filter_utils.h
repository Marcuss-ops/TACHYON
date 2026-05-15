#pragma once

#include "tachyon/api.h"
#include <string>
#include <vector>
#include <utility>

namespace tachyon::core::audio {

/**
 * @brief Configuration for the amix filter.
 */
struct AmixConfig {
    int inputs{1};
    double offset_seconds{0.0};
    double volume_db{0.0};
};

/**
 * @brief Utilities for generating FFmpeg-compatible audio filter expressions.
 * This class centralizes domain knowledge about how Tachyon structures audio processing.
 */
class TACHYON_API AudioFilterUtils {
public:
    /**
     * @brief Builds a gate expression for volume evaluation.
     * @param ranges Vector of (start, end) pairs to mute or unmute.
     * @param inverted If true, volume is 1 only within ranges. If false, volume is 0 within ranges.
     */
    static std::string build_gate_expr_from_ranges(
        const std::vector<std::pair<double, double>>& ranges,
        bool inverted);

    /**
     * @brief Builds a gate expression that allows audio only for a specific duration.
     */
    static std::string build_intro_only_gate_expr(double duration);

    /**
     * @brief Builds an amix filter string.
     */
    static std::string build_amix_filter(const AmixConfig& config);

    /**
     * @brief Builds a background music filter with fade in/out and volume adjustment.
     */
    static std::string build_background_music_filter(
        double duration,
        double fade_in_seconds,
        double fade_out_seconds,
        double volume_db);

    /**
     * @brief Builds an adelay filter string.
     */
    static std::string build_audio_delay_filter(double delay_seconds);

    /**
     * @brief Builds an acrossfade filter string.
     */
    static std::string build_acrossfade_filter(double duration_seconds);

    /**
     * @brief Builds a concat audio filter string.
     */
    static std::string build_concat_audio_filter(int input_count);

    /**
     * @brief Builds the complex filtergraph string for a master audio bake.
     */
    static std::string build_bake_filter(
        const std::vector<std::pair<double, double>>& gate_ranges,
        bool has_voiceover,
        double voiceover_offset_seconds,
        bool has_background_music,
        double music_volume_db);

    /**
     * @brief Constructs the full FFmpeg command for audio baking.
     */
    static std::vector<std::string> build_bake_command(
        const std::string& base_video_path,
        const std::string* voiceover_path,
        const std::string* background_music_path,
        const std::string& output_path,
        const std::vector<std::pair<double, double>>& gate_ranges,
        double voiceover_offset_seconds,
        double music_volume_db,
        int sample_rate,
        bool use_aac);
};

} // namespace tachyon::core::audio
