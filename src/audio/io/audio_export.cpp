#include "tachyon/audio/processing/loudness_meter.h"
#include "tachyon/audio/io/audio_export.h"
#include "tachyon/audio/io/audio_decoder.h"
#include "tachyon/audio/processing/audio_processor.h"
#include "tachyon/core/animation/keyframe.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"

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

float AudioExporter::evaluate_fade_at_time(const AudioTrackSpec& track, double time, double chunk_duration) const {
    (void)chunk_duration;
    float fade_factor = 1.0f;
    
    // Fade in
    if (track.fade_in_duration > 0.0 && time < track.fade_in_duration) {
        fade_factor *= static_cast<float>(time / track.fade_in_duration);
    }
    
    // Fade out (calculated relative to track end)
    if (track.fade_out_duration > 0.0) {
        // Find the track instance to get its decoder duration
        double decoder_duration = 0.0;
        for (const auto& t : m_tracks) {
            if (&t.spec == &track) {
                decoder_duration = t.decoder->duration();
                break;
            }
        }
        
        double track_end = get_track_end_time(track, decoder_duration);
        double time_to_end = track_end - time;
        if (time_to_end < track.fade_out_duration) {
            fade_factor *= static_cast<float>(time_to_end / track.fade_out_duration);
        }
    }
    
    return fade_factor;
}

double AudioExporter::get_track_end_time(const AudioTrackSpec& track, double decoder_duration) const {
    // Calcolo durata considerando trim e playback speed
    double duration = decoder_duration;
    
    // Applica trim
    if (track.out_point_seconds > 0.0 && track.out_point_seconds > track.in_point_seconds) {
        duration = track.out_point_seconds;
    }
    
    duration -= track.in_point_seconds;
    duration = std::max(0.0, duration);
    
    // Applica playback speed
    duration /= std::max(0.01f, track.playback_speed);
    
    // Aggiungi offset
    return duration + track.start_offset_seconds;
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

bool AudioExporter::export_to(const std::filesystem::path& output_path, const AudioExportConfig& config, double start_time, double duration, std::atomic<bool>* cancel_flag) {
    if (m_tracks.empty()) return false;

    // Reset loudness meter
    m_loudness_meter.reset();

    AudioEncoder encoder;
    if (!encoder.open(output_path, config)) return false;

    double max_duration = 0.0;
    if (duration > 0.0) {
        max_duration = start_time + duration;
    } else {
        for (const auto& track : m_tracks) {
            double track_duration = get_track_end_time(track.spec, track.decoder->duration());
            max_duration = std::max(max_duration, track_duration);
        }
    }

    AudioProcessor processor;
    const double chunk_duration = 1.0; 
    std::vector<float> mix_buffer;
    std::vector<float> track_buffer;

    for (double t = start_time; t < max_duration; t += chunk_duration) {
        if (cancel_flag && cancel_flag->load()) return false;
        double current_chunk = std::min(chunk_duration, max_duration - t);
        
        processor.clear_tracks();
        for (auto& track : m_tracks) {
            // Volume and fade are evaluated per chunk to support keyframes
            AudioTrackSpec instance_spec = track.spec;
            float volume = evaluate_volume_at_time(track.spec, t);
            float fade = evaluate_fade_at_time(track.spec, t, current_chunk);
            instance_spec.volume = volume * fade;
            instance_spec.pan = evaluate_pan_at_time(track.spec, t);
            
            // Add to processor. Trim and speed are handled within AudioProcessor::mix_track
            processor.add_track(track.decoder, instance_spec);
        }

        processor.process(t, current_chunk, config.sample_rate, mix_buffer);

        if (!encoder.encode_interleaved_float(mix_buffer)) {
            return false;
        }
        
        // Misura loudness del mix finale
        m_loudness_meter.process(mix_buffer.data(), 
                                 static_cast<int>(mix_buffer.size() / 2), 
                                 config.sample_rate);
    }

    // Salva measurement finale
    m_loudness_measurement = m_loudness_meter.current();
    
    // Log risultato LUFS
    std::cout << "[Audio Export] Integrated LUFS: " << m_loudness_measurement.integrated_lufs 
              << " dB, True Peak: " << m_loudness_measurement.true_peak_dbfs << " dBFS\n";

    return encoder.flush();
}

void AudioExporter::apply_pan(float* interleaved_samples, std::size_t sample_count, float pan) {
    if (!interleaved_samples || sample_count == 0) return;
    
    // Pan va da -1.0 (full left) a 1.0 (full right)
    // Normalizza a 0.0-1.0 per il gain di ogni canale
    float left_gain = 1.0f - std::max(0.0f, pan);
    float right_gain = 1.0f + std::min(0.0f, pan);
    
    for (std::size_t i = 0; i < sample_count; ++i) {
        interleaved_samples[i * 2] *= left_gain;      // Left
        interleaved_samples[i * 2 + 1] *= right_gain; // Right
    }
}

bool has_any_audio(const RenderPlan& plan) {
    if (!plan.scene_spec) return false;
    
    if (!plan.output.profile.audio.tracks.empty()) return true;
    
    // Find the composition target
    for (const auto& comp : plan.scene_spec->compositions) {
        if (comp.id == plan.composition_target) {
            return !comp.audio_tracks.empty();
        }
    }
    return false;
}

bool export_plan_audio(const RenderPlan& plan, const std::filesystem::path& output_path, std::atomic<bool>* cancel_flag) {
    if (!plan.scene_spec) return false;

    AudioExporter exporter;
    bool found_comp = false;
    
    for (const auto& comp : plan.scene_spec->compositions) {
        if (comp.id == plan.composition_target) {
            for (const auto& track : comp.audio_tracks) {
                exporter.add_track(track);
            }
            found_comp = true;
            break;
        }
    }

    // Also add tracks from job output profile
    for (const auto& job_track : plan.output.profile.audio.tracks) {
        AudioTrackSpec spec;
        spec.source_path = job_track.source_path;
        spec.volume = static_cast<float>(job_track.volume);
        spec.start_offset_seconds = job_track.start_offset_seconds;
        exporter.add_track(spec);
        found_comp = true;
    }

    if (!found_comp) return false;

    AudioExportConfig config;
    config.sample_rate = 48000;
    config.channels = 2;
    config.bitrate_kbps = 192;
    
    // Determine temporal range from frame_range
    double fps = plan.composition.frame_rate.value();
    if (fps <= 0.0) fps = 24.0;
    
    double start_time = static_cast<double>(plan.frame_range.start) / fps;
    double end_time = static_cast<double>(plan.frame_range.end + 1) / fps;
    
    // We mix from 0 in the mix loop, but AudioProcessor::process takes start_time
    // Wait, AudioExporter::export_to mixes from 0 to max_duration.
    // I should probably pass start/end to export_to.
    
    if (output_path.extension() == ".wav") {
        config.codec = "pcm_s16le";
    }

    return exporter.export_to(output_path, config, start_time, end_time - start_time, cancel_flag);
}

} // namespace tachyon::audio

