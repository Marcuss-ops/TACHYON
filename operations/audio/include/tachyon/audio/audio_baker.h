#pragma once

#include "tachyon/core/media/media_error.h"
#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <utility>

namespace tachyon::audio {

/**
 * @brief Configuration for the master audio baking process.
 * Derived from ruststream-core/audio/audio_bake.rs
 */
struct AudioBakeConfig {
    std::filesystem::path base_video_path;
    std::optional<std::filesystem::path> voiceover_path;
    std::optional<std::filesystem::path> background_music_path;
    std::filesystem::path output_path;

    double voiceover_offset_seconds{0.0};
    std::vector<std::pair<double, double>> gate_ranges;
    double music_volume_db{-10.0};
    int sample_rate{48000};
    bool use_aac{true};
};

/**
 * @brief Orchestrates the final audio mixdown (baking) using FFmpeg.
 * 
 * This class builds and executes the FFmpeg command necessary to merge 
 * multiple audio sources into a single master track, applying necessary 
 * filters like ducking, volume normalization, and synchronization.
 */
class AudioBaker {
public:
    /**
     * @brief Executes the audio baking process.
     * @param config The baking configuration.
     * @return A MediaResult indicating success or failure.
     */
    static core::MediaResult<void> bake_master_audio(const AudioBakeConfig& config);

private:
    /**
     * @brief Constructs the complex filtergraph string for FFmpeg.
     */
    static std::string build_bake_filter(const AudioBakeConfig& config);
    
    /**
     * @brief Constructs the full FFmpeg command as a list of arguments.
     */
    static std::vector<std::string> build_bake_command(const AudioBakeConfig& config);
};

} // namespace tachyon::audio
