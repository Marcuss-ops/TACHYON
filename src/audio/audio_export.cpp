#include "tachyon/audio/audio_export.h"
#include "tachyon/audio/audio_decoder.h"
#include "tachyon/audio/audio_processor.h"
#include "tachyon/core/animation/keyframe.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <filesystem>

namespace tachyon::audio {

static float evaluate_keyframe_value(const std::vector<animation::Keyframe<float>>& keyframes, double time) {
    if (keyframes.empty()) return 1.0f;
    if (keyframes.size() == 1) return keyframes[0].value;

    std::size_t idx = 0;
    for (std::size_t i = 0; i < keyframes.size(); ++i) {
        if (keyframes[i].time <= time) idx = i;
        else break;
    }

    if (idx >= keyframes.size() - 1) return keyframes.back().value;

    const auto& kf0 = keyframes[idx];
    const auto& kf1 = keyframes[idx + 1];

    if (kf0.out_mode == animation::InterpolationMode::Hold) return kf0.value;
    
    double t = (time - kf0.time) / (kf1.time - kf0.time);
    t = std::max(0.0, std::min(1.0, t));

    if (kf0.out_mode == animation::InterpolationMode::Linear) {
        return static_cast<float>(kf0.value + (kf1.value - kf0.value) * t);
    }

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

AudioExporter::AudioExporter() = default;
AudioExporter::~AudioExporter() { clear_tracks(); }

void AudioExporter::add_track(const AudioTrackSpec& track_spec) {
    auto decoder = std::make_shared<AudioDecoder>();
    if (decoder->open(track_spec.source_path)) {
        m_tracks.push_back({std::move(decoder), track_spec});
    }
}

void AudioExporter::clear_tracks() { m_tracks.clear(); }

bool AudioExporter::export_to(const std::filesystem::path& output_path, const AudioExportConfig& config) {
    if (m_tracks.empty()) return false;

    AudioEncoder encoder;
    if (!encoder.open(output_path, config)) return false;

    double max_duration = 0.0;
    for (const auto& track : m_tracks) {
        double track_duration = (track.decoder->duration() / std::max(0.01f, track.spec.playback_speed)) + track.spec.start_offset_seconds;
        max_duration = std::max(max_duration, track_duration);
    }

    AudioProcessor processor;
    const double chunk_duration = 1.0; 
    std::vector<float> mix_buffer;

    for (double t = 0.0; t < max_duration; t += chunk_duration) {
        double current_chunk = std::min(chunk_duration, max_duration - t);
        
        processor.clear_tracks();
        for (const auto& track : m_tracks) {
            AudioTrackSpec instance_spec = track.spec;
            instance_spec.volume = evaluate_volume_at_time(track.spec, t);
            instance_spec.pan = evaluate_pan_at_time(track.spec, t);
            processor.add_track(track.decoder, instance_spec);
        }

        processor.process(t, current_chunk, config.sample_rate, mix_buffer);

        if (!encoder.encode_interleaved_float(mix_buffer)) {
            return false;
        }
    }

    return encoder.flush();
}

} // namespace tachyon::audio
