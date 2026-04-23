#include "tachyon/audio/audio_export.h"
#include "tachyon/audio/audio_decoder.h"
#include "tachyon/core/animation/keyframe.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <filesystem>

namespace tachyon::audio {

static float evaluate_keyframe_value(const std::vector<animation::Keyframe<float>>& keyframes, double time) {
    if (keyframes.empty()) return 1.0f;
    if (keyframes.size() == 1) return keyframes[0].value;

    // Find the surrounding keyframes
    std::size_t idx = 0;
    for (std::size_t i = 0; i < keyframes.size(); ++i) {
        if (keyframes[i].time <= time) idx = i;
        else break;
    }

    if (idx >= keyframes.size() - 1) return keyframes.back().value;

    const auto& kf0 = keyframes[idx];
    const auto& kf1 = keyframes[idx + 1];

    if (kf0.out_mode == animation::InterpolationMode::Hold) return kf0.value;
    if (kf0.out_mode == animation::InterpolationMode::Linear) {
        double t = (time - kf0.time) / (kf1.time - kf0.time);
        t = std::max(0.0, std::min(1.0, t));
        return static_cast<float>(kf0.value + (kf1.value - kf0.value) * t);
    }

    double t = (time - kf0.time) / (kf1.time - kf0.time);
    t = std::max(0.0, std::min(1.0, t));

    if (kf0.out_mode == animation::InterpolationMode::Bezier) {
        const double eased = kf0.bezier.evaluate(t);
        return static_cast<float>(kf0.value + (kf1.value - kf0.value) * eased);
    }

    return static_cast<float>(kf0.value + (kf1.value - kf0.value) * t);
}

float AudioExporter::evaluate_volume_at_time(const AudioTrackSpec& track, double time) const {
    if (track.volume_keyframes.empty()) return track.volume;
    return evaluate_keyframe_value(track.volume_keyframes, time);
}

float AudioExporter::evaluate_pan_at_time(const AudioTrackSpec& track, double time) const {
    if (track.pan_keyframes.empty()) return track.pan;
    return evaluate_keyframe_value(track.pan_keyframes, time);
}

void AudioExporter::apply_pan(float* interleaved_samples, std::size_t sample_count, float pan) {
    // Pan: -1.0 = left, 0.0 = center, 1.0 = right
    float left_gain = (pan <= 0.0f) ? 1.0f : (1.0f - pan) * 0.5f;
    float right_gain = (pan >= 0.0f) ? 1.0f : (1.0f + pan) * 0.5f;

    for (std::size_t i = 0; i < sample_count; i += 2) {
        interleaved_samples[i] *= left_gain;      // Left channel
        interleaved_samples[i + 1] *= right_gain; // Right channel
    }
}

static void apply_gain(float* samples, std::size_t sample_count, float gain_db) {
    float linear_gain = std::pow(10.0f, gain_db / 20.0f);
    for (std::size_t i = 0; i < sample_count; ++i) {
        samples[i] *= linear_gain;
    }
}

static void apply_fade_in(float* samples, std::size_t sample_count, double fade_duration, double chunk_start, double chunk_duration, int sample_rate) {
    (void)chunk_duration;
    std::size_t fade_samples = static_cast<std::size_t>(fade_duration * sample_rate);
    if (fade_samples == 0) return;

    for (std::size_t i = 0; i < sample_count; ++i) {
        double sample_time = chunk_start + static_cast<double>(i / 2) / sample_rate;
        if (sample_time < fade_duration) {
            float fade_factor = static_cast<float>(sample_time / fade_duration);
            samples[i] *= fade_factor;
        } else {
            break;
        }
    }
}

static void apply_fade_out(float* samples, std::size_t sample_count, double fade_start, double fade_duration, double chunk_start, double chunk_duration, int sample_rate) {
    (void)chunk_duration;
    for (std::size_t i = 0; i < sample_count; ++i) {
        double sample_time = chunk_start + static_cast<double>(i / 2) / sample_rate;
        if (sample_time >= fade_start) {
            float fade_factor = static_cast<float>(1.0 - (sample_time - fade_start) / fade_duration);
            fade_factor = std::max(0.0f, std::min(1.0f, fade_factor));
            samples[i] *= fade_factor;
        }
    }
}

AudioExporter::AudioExporter() = default;

AudioExporter::~AudioExporter() {
    clear_tracks();
}

void AudioExporter::add_track(const AudioTrackSpec& track_spec) {
    TrackInstance inst;
    inst.spec = track_spec;
    inst.decoder = std::make_shared<AudioDecoder>();
    if (inst.decoder->open(track_spec.source_path)) {
        m_tracks.push_back(inst);
    }
}

void AudioExporter::clear_tracks() {
    m_tracks.clear();
}

bool AudioExporter::export_to(const std::filesystem::path& output_path, const AudioExportConfig& config) {
    if (m_tracks.empty()) return false;

    AudioEncoder encoder;
    if (!encoder.open(output_path, config)) return false;

    double duration = 0.0;
    for (const auto& track : m_tracks) {
        if (track.decoder->duration() > duration) {
            duration = track.decoder->duration();
        }
    }

    const double chunk_duration = 1.0; // Process 1 second at a time
    for (double t = 0.0; t < duration; t += chunk_duration) {
        double current_duration = std::min(chunk_duration, duration - t);
        std::size_t samples_needed = static_cast<std::size_t>(current_duration * config.sample_rate);

        // Mix all tracks
        std::vector<float> temp_buffer(samples_needed * 2, 0.0f);
        for (auto& track : m_tracks) {
            if (!track.decoder->is_open()) continue;

            double track_start = t - track.spec.start_offset_seconds;
            auto samples = track.decoder->decode_range(std::max(0.0, track_start), current_duration);

            float volume = evaluate_volume_at_time(track.spec, t);
            float pan = evaluate_pan_at_time(track.spec, t);

            // Apply volume and pan
            for (std::size_t i = 0; i < samples.size() && i < temp_buffer.size(); ++i) {
                temp_buffer[i] += samples[i] * volume;
            }
            apply_pan(temp_buffer.data(), std::min(samples.size(), temp_buffer.size()), pan);

            // Apply effects
            for (const auto& effect : track.spec.effects) {
                if (effect.type == "gain" && effect.gain_db.has_value()) {
                    apply_gain(temp_buffer.data(), temp_buffer.size(), *effect.gain_db);
                } else if (effect.type == "fade_in" && effect.start_time.has_value() && effect.duration.has_value()) {
                    apply_fade_in(temp_buffer.data(), temp_buffer.size(), *effect.duration, t, current_duration, config.sample_rate);
                } else if (effect.type == "fade_out" && effect.start_time.has_value() && effect.duration.has_value()) {
                    apply_fade_out(temp_buffer.data(), temp_buffer.size(), *effect.start_time, *effect.duration, t, current_duration, config.sample_rate);
                }
            }
        }

        if (!encoder.encode_interleaved_float(temp_buffer)) {
            return false;
        }
    }

    if (!encoder.flush()) return false;
    return true;
}

} // namespace tachyon::audio
