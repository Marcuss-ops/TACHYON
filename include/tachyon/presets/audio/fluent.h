#pragma once

#include "tachyon/core/spec/schema/contracts/shared_contracts.h"
#include "tachyon/core/spec/schema/properties/property_spec.h"
#include <string>
#include <vector>
#include <optional>

namespace tachyon::presets::audio {

/**
 * @brief Fluent builder for audio tracks (Remotion-style API).
 * 
 * Usage:
 * @code
 * auto track = audio::track("voice")
 *     .source("voiceover.wav")
 *     .volume(1.0)
 *     .fade_in(0.2)
 *     .fade_out(0.4)
 *     .normalize_lufs(-14.0)
 *     .build();
 * @endcode
 */
class AudioTrackBuilder {
    AudioTrackSpec spec_;
    
public:
    explicit AudioTrackBuilder(std::string id) {
        spec_.id = std::move(id);
    }
    
    // Implicit conversion from string (for simple case: track("voice"))
    AudioTrackBuilder& source(std::string path) {
        spec_.source_path = std::move(path);
        return *this;
    }
    
    AudioTrackBuilder& volume(float v) {
        spec_.volume = v;
        return *this;
    }
    
    AudioTrackBuilder& pan(float p) {
        spec_.pan = p;
        return *this;
    }
    
    AudioTrackBuilder& start(double offset) {
        spec_.start_offset_seconds = offset;
        return *this;
    }
    
    AudioTrackBuilder& trim(double in, double out) {
        spec_.in_point_seconds = in;
        spec_.out_point_seconds = out;
        return *this;
    }
    
    AudioTrackBuilder& fade_in(double duration) {
        spec_.fade_in_duration = duration;
        return *this;
    }
    
    AudioTrackBuilder& fade_out(double duration) {
        spec_.fade_out_duration = duration;
        return *this;
    }
    
    AudioTrackBuilder& speed(float s) {
        spec_.playback_speed = s;
        return *this;
    }
    
    AudioTrackBuilder& pitch_shift(float ps) {
        spec_.pitch_shift = ps;
        return *this;
    }
    
    AudioTrackBuilder& pitch_correct(bool enable) {
        spec_.pitch_correct = enable;
        return *this;
    }
    
    AudioTrackBuilder& normalize_lufs(float target = -14.0f) {
        spec_.normalize_to_lufs = true;
        spec_.target_lufs = target;
        return *this;
    }
    
    AudioTrackBuilder& volume_keyframe(double time, float value) {
        spec_.volume_keyframes.push_back({time, value});
        return *this;
    }
    
    AudioTrackBuilder& pan_keyframe(double time, float value) {
        spec_.pan_keyframes.push_back({time, value});
        return *this;
    }

    AudioTrackBuilder& effect(const AudioEffectSpec& fx) {
        spec_.effects.push_back(fx);
        return *this;
    }

    AudioTrackBuilder& effect_fade_in(double duration) {
        AudioEffectSpec fx;
        fx.type = "fade_in";
        fx.duration = duration;
        spec_.effects.push_back(std::move(fx));
        return *this;
    }

    AudioTrackBuilder& effect_fade_out(double duration) {
        AudioEffectSpec fx;
        fx.type = "fade_out";
        fx.duration = duration;
        spec_.effects.push_back(std::move(fx));
        return *this;
    }

    AudioTrackBuilder& effect_gain(float db) {
        AudioEffectSpec fx;
        fx.type = "gain";
        fx.gain_db = db;
        spec_.effects.push_back(std::move(fx));
        return *this;
    }
    
    [[nodiscard]] AudioTrackSpec build() const {
        return spec_;
    }
    
    [[nodiscard]] operator AudioTrackSpec() const {
        return spec_;
    }
};

// ---------------------------------------------------------------------------
// Factory function for Remotion-style audio track creation
// ---------------------------------------------------------------------------

inline AudioTrackBuilder track(std::string id) {
    return AudioTrackBuilder(std::move(id));
}

// Also support creating track directly from source path
inline AudioTrackBuilder track(std::string id, std::string source_path) {
    AudioTrackBuilder builder(std::move(id));
    builder.source(std::move(source_path));
    return builder;
}

} // namespace tachyon::presets::audio
