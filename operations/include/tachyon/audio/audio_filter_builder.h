#pragma once

#include <string>
#include <vector>

namespace tachyon::audio {

/**
 * @brief Utility for building FFmpeg audio filter strings.
 * Derived from ruststream-core/audio/audio_mix.rs
 */
class AudioFilterBuilder {
public:
    struct AmixConfig {
        double offset_seconds{0.0};
        double volume_db{0.0};
        int inputs{2};
    };

    /**
     * @brief Builds an amix filter string with optional offset and volume.
     */
    static std::string build_amix_filter(const AmixConfig& config);
    
    /**
     * @brief Builds a filter string for background music with fades and looping.
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
     * @brief Builds an acrossfade filter string between two streams.
     */
    static std::string build_acrossfade_filter(double duration_seconds);
    
    /**
     * @brief Builds a concat filter string for audio streams.
     */
    static std::string build_concat_audio_filter(int input_count);
};

} // namespace tachyon::audio
