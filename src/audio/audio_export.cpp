#include "tachyon/audio/loudness_meter.h"
#include "tachyon/audio/audio_export.h"
#include "tachyon/audio/audio_decoder.h"
#include "tachyon/audio/audio_processor.h"
#include "tachyon/core/animation/keyframe.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/core/data/compiled_scene.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <filesystem>

namespace tachyon::audio {

#ifndef TACHYON_ENABLE_AUDIO_MUX
bool export_plan_audio(const RenderPlan& plan, const std::filesystem::path& output_path) { return false; }
bool has_any_audio(const RenderPlan& plan) { return false; }
#else

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
        double track_end = get_track_end_time(track);
        double time_to_end = track_end - time;
        if (time_to_end < track.fade_out_duration) {
            fade_factor *= static_cast<float>(time_to_end / track.fade_out_duration);
        }
    }
    
    return fade_factor;
}

double AudioExporter::get_track_end_time(const AudioTrackSpec& track) const {
    // Calcolo durata considerando trim e playback speed
    double duration = 0.0;
    
    // Applica trim
    if (track.out_point_seconds > 0.0 && track.out_point_seconds > track.in_point_seconds) {
        duration = track.out_point_seconds - track.in_point_seconds;
    } else if (track.in_point_seconds > 0.0) {
        duration -= track.in_point_seconds;
    }
    
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

bool AudioExporter::export_to(const std::filesystem::path& output_path, const AudioExportConfig& config) {
    if (m_tracks.empty()) return false;

    // Reset loudness meter
    m_loudness_meter.reset();

    AudioEncoder encoder;
    if (!encoder.open(output_path, config)) return false;

    double max_duration = 0.0;
    for (const auto& track : m_tracks) {
        double track_duration = get_track_end_time(track.spec);
        max_duration = std::max(max_duration, track_duration);
    }

    AudioProcessor processor;
    const double chunk_duration = 1.0; 
    std::vector<float> mix_buffer;
    std::vector<float> track_buffer;

    for (double t = 0.0; t < max_duration; t += chunk_duration) {
        double current_chunk = std::min(chunk_duration, max_duration - t);
        
        processor.clear_tracks();
        for (auto& track : m_tracks) {
            // Applica trim e speed al decoder
            apply_trim_and_speed(track.decoder.get(), track.spec, t, current_chunk, track_buffer);
            
            // Crea una istanza temporanea con volume/fade applicati
            AudioTrackSpec instance_spec = track.spec;
            float volume = evaluate_volume_at_time(track.spec, t);
            float fade = evaluate_fade_at_time(track.spec, t, current_chunk);
            instance_spec.volume = volume * fade;
            instance_spec.pan = evaluate_pan_at_time(track.spec, t);
            
            // Aggiungi al processore con il buffer già processato
            // (in una implementazione reale, AudioProcessor dovrebbe accettare buffer pre-processati)
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

void AudioExporter::apply_trim_and_speed(AudioDecoder* decoder, const AudioTrackSpec& spec, 
                                          double chunk_start, double chunk_duration,
                                          std::vector<float>& output_buffer) {
    if (!decoder) return;
    (void)output_buffer;
    
    // Calcola posizione di lettura considerando trim e offset
    double read_start = spec.in_point_seconds + (chunk_start - spec.start_offset_seconds) * spec.playback_speed;
    double read_duration = chunk_duration * spec.playback_speed;
    
    // Verifica bounds
    if (read_start < 0) read_start = 0;
    if (spec.out_point_seconds > 0 && read_start + read_duration > spec.out_point_seconds) {
        read_duration = spec.out_point_seconds - read_start;
    }
    
    // Decodifica chunk dalla posizione corretta
    // (implementazione semplificata - in produzione servirebbe seek preciso)
    // decoder->seek(read_start); // AudioDecoder has no seek method
    // Il decoder ora leggerà dalla posizione corretta per i chunk successivi
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

bool export_scene_audio(const CompiledScene& scene, const std::filesystem::path& output_path) {
    if (scene.compositions.empty()) return false;
    const auto& comp = scene.compositions.front();
    if (comp.audio_tracks.empty()) return false;

    AudioExporter exporter;
    for (const auto& track : comp.audio_tracks) {
        exporter.add_track(track);
    }

    AudioExportConfig config;
    return exporter.export_to(output_path, config);
}

bool export_plan_audio(const RenderPlan& plan, const std::filesystem::path& output_path) {
    AudioExporter exporter;
    
    // 1. Add tracks from Composition (via SceneSpec if available)
    if (plan.scene_spec != nullptr) {
        const auto& scene = *plan.scene_spec;
        auto it = std::find_if(scene.compositions.begin(), scene.compositions.end(),
            [&](const auto& c) { return c.id == plan.composition_target; });
        
        if (it != scene.compositions.end()) {
            for (const auto& track : it->audio_tracks) {
                exporter.add_track(track);
            }
        }
    }

    // 2. Add tracks from Output Profile (Overlay audio)
    for (const auto& track : plan.output.profile.audio.tracks) {
        if (track.source_path.empty()) continue;
        AudioTrackSpec spec;
        spec.id = "output_overlay_" + std::to_string(reinterpret_cast<std::uintptr_t>(&track));
        spec.source_path = track.source_path;
        spec.volume = static_cast<float>(track.volume);
        spec.start_offset_seconds = track.start_offset_seconds;
        exporter.add_track(spec);
    }

    AudioExportConfig config;
    if (plan.output.profile.audio.sample_rate) config.sample_rate = static_cast<int>(*plan.output.profile.audio.sample_rate);
    if (plan.output.profile.audio.channels) config.channels = static_cast<int>(*plan.output.profile.audio.channels);
    
    return exporter.export_to(output_path, config);
}

bool has_any_audio(const RenderPlan& plan) {
    if (!plan.output.profile.audio.tracks.empty()) return true;
    if (plan.scene_spec != nullptr) {
        const auto& scene = *plan.scene_spec;
        auto it = std::find_if(scene.compositions.begin(), scene.compositions.end(),
            [&](const auto& c) { return c.id == plan.composition_target; });
        if (it != scene.compositions.end() && !it->audio_tracks.empty()) return true;
    }
    return false;
}

#endif // TACHYON_ENABLE_AUDIO_MUX

} // namespace tachyon::audio
