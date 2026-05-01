#pragma once

#include "tachyon/core/spec/schema/contracts/shared_contracts.h"
#include "tachyon/core/animation/keyframe.h"
#include <string>
#include <vector>
#include <optional>

namespace tachyon::presets::audio {

/**
 * @brief Fluent builder for AudioTrackSpec.
 * 
 * Provides a clean, chainable API for constructing audio tracks:
 * 
 * @code
 * auto track = audio::track("voice")
 *     .source("voice.wav")
 *     .volume(1.0)
 *     .fade_in(0.2)
 *     .fade_out(0.3)
 *     .normalize_lufs(-14.0);
 * @endcode
 */
class TrackBuilder {
    spec::AudioTrackSpec spec_;
public:
    explicit TrackBuilder(std::string id) {
        spec_.id = std::move(id);
    }

    TrackBuilder& source(std::string path) {
        spec_.source_path = std::move(path);
        return *this;
    }

    TrackBuilder& volume(float v) {
        spec_.volume = v;
        return *this;
    }

    TrackBuilder& pan(float p) {
        spec_.pan = p;
        return *this;
    }

    TrackBuilder& start(double offset_seconds) {
        spec_.start_offset_seconds = offset_seconds;
        return *this;
    }

    TrackBuilder& trim(double in_point, double out_point) {
        spec_.in_point_seconds = in_point;
        spec_.out_point_seconds = out_point;
        return *this;
    }

    TrackBuilder& fade_in(double duration) {
        spec_.fade_in_duration = duration;
        return *this;
    }

    TrackBuilder& fade_out(double duration) {
        spec_.fade_out_duration = duration;
        return *this;
    }

    TrackBuilder& speed(float s) {
        spec_.playback_speed = s;
        return *this;
    }

    TrackBuilder& pitch_shift(float ps) {
        spec_.pitch_shift = ps;
        return *this;
    }

    TrackBuilder& pitch_correct(bool enabled) {
        spec_.pitch_correct = enabled;
        return *this;
    }

    TrackBuilder& normalize_lufs(float target = -14.0f) {
        spec_.normalize_to_lufs = true;
        spec_.target_lufs = target;
        return *this;
    }

    TrackBuilder& add_volume_keyframe(double time, float value) {
        spec_.volume_keyframes.push_back({static_cast<float>(time), value});
        return *this;
    }

    TrackBuilder& add_pan_keyframe(double time, float value) {
        spec_.pan_keyframes.push_back({static_cast<float>(time), value});
        return *this;
    }

    TrackBuilder& add_effect(spec::AudioEffectSpec effect) {
        spec_.effects.push_back(std::move(effect));
        return *this;
    }

    [[nodiscard]] spec::AudioTrackSpec build() const {
        return spec_;
    }

    [[nodiscard]] spec::AudioTrackSpec build() && {
        return std::move(spec_);
    }
};

/**
 * @brief Create a new audio track builder.
 * 
 * @param id Unique identifier for the track
 * @return TrackBuilder for fluent configuration
 */
inline TrackBuilder track(std::string id) {
    return TrackBuilder(std::move(id));
}

} // namespace tachyon::presets::audio
