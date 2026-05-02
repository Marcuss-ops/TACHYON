#include "tachyon/audio/processing/audio_processor.h"
#include "tachyon/audio/core/dsp_nodes.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace tachyon::audio {

AudioProcessor::AudioProcessor(AudioProcessorConfig config) : config_(std::move(config)) {}
AudioProcessor::~AudioProcessor() = default;

void AudioProcessor::add_track(std::shared_ptr<AudioDecoder> decoder, const spec::AudioTrackSpec& spec, const std::string& bus_id) {
    if (decoder) {
        TrackInstance track;
        track.decoder = std::move(decoder);
        track.spec = spec;
        track.bus_id = bus_id;
        m_tracks.push_back(std::move(track));
        
        // Setup effect nodes for the newly added track
        setup_track_effects(m_tracks.back());
    }
}

void AudioProcessor::clear_tracks() {
    m_tracks.clear();
}

void AudioProcessor::process(double startTimeSeconds, double durationSeconds, int sampleRate, std::vector<float>& out_stereo_pcm) {
    if (durationSeconds < 0.0) {
        out_stereo_pcm.clear();
        return;
    }

    const std::size_t target_samples = static_cast<std::size_t>(std::ceil(durationSeconds * sampleRate));
    out_stereo_pcm.assign(target_samples * 2, 0.0f);

    for (const auto& track : m_tracks) {
        mix_track(track, startTimeSeconds, durationSeconds, sampleRate, out_stereo_pcm);
    }

    m_graph.master().process(out_stereo_pcm.data(), static_cast<int>(target_samples));
}

// ---------------------------------------------------------------------------
// Fast resampling for scrub preview
// ---------------------------------------------------------------------------
void AudioProcessor::resample_for_scrub(const std::vector<float>& input, float speed, std::vector<float>& output) {
    if (input.empty() || std::abs(speed) < 0.01f) {
        output.clear();
        return;
    }

    const float abs_speed = std::abs(speed);
    const std::size_t output_size = static_cast<std::size_t>(input.size() / abs_speed);
    output.resize(output_size);

    for (std::size_t i = 0; i < output_size; ++i) {
        float src_pos = static_cast<float>(i) * abs_speed;
        int idx = static_cast<int>(src_pos);
        float frac = src_pos - idx;

        idx = std::clamp(idx, 0, static_cast<int>(input.size()) - 2);
        output[i] = input[idx] * (1.0f - frac) + input[idx + 1] * frac;
    }
}

// ---------------------------------------------------------------------------
// Reverse playback
// ---------------------------------------------------------------------------
void AudioProcessor::reverse_audio(std::vector<float>& audio) {
    if (audio.size() < 2) return;
    std::reverse(audio.begin(), audio.end());
}

// ---------------------------------------------------------------------------
// WSOLA time-stretch implementation
// ---------------------------------------------------------------------------
void AudioProcessor::apply_time_stretch(const std::vector<float>& input, float speed, std::vector<float>& output) {
    if (speed <= 0.0f) {
        output.clear();
        return;
    }
    if (std::abs(speed - 1.0f) < 0.01f) {
        output = input;
        return;
    }

    constexpr int channels = 2;
    if (input.empty() || (input.size() % channels) != 0) {
        output.clear();
        return;
    }

    const std::size_t input_frames = input.size() / channels;
    const std::size_t output_frames = static_cast<std::size_t>(std::ceil(static_cast<float>(input_frames) / speed));
    if (output_frames == 0) {
        output.clear();
        return;
    }

    // --- WSOLA parameters ---
    constexpr int frame_ms = 30;
    const int frame_size = (kInternalSampleRate * frame_ms) / 1000 * channels;
    const int hop_out = frame_size / 4;
    const int hop_in  = static_cast<int>(std::round(static_cast<float>(hop_out) * speed));

    if (frame_size <= 0 || hop_in <= 0 || hop_out <= 0) {
        output.clear();
        return;
    }

    output.assign(output_frames * channels, 0.0f);

    std::vector<float> window(frame_size);
    for (int i = 0; i < frame_size; ++i) {
        window[i] = 0.5f - 0.5f * std::cos(2.0f * 3.14159265358979f * static_cast<float>(i) / static_cast<float>(frame_size - 1));
    }

    constexpr int search_ms = 15;
    const int search_range = (kInternalSampleRate * search_ms) / 1000 * channels;

    std::size_t in_pos = 0;
    std::size_t out_pos = 0;

    while (out_pos + frame_size <= output.size() && in_pos < input.size()) {
        int best_offset = 0;
        if (in_pos >= static_cast<std::size_t>(frame_size + search_range)) {
            float best_corr = -1e30f;
            const std::size_t search_start = in_pos - search_range;
            const std::size_t search_end = std::min(in_pos + search_range, input.size() - frame_size);

            for (std::size_t candidate = search_start; candidate <= search_end; candidate += channels) {
                float corr = 0.0f;
                for (int i = 0; i < frame_size; ++i) {
                    corr += input[in_pos + i] * input[candidate + i];
                }
                if (corr > best_corr) {
                    best_corr = corr;
                    best_offset = static_cast<int>(candidate) - static_cast<int>(in_pos);
                }
            }
        }

        const std::size_t read_pos = static_cast<std::size_t>(static_cast<int>(in_pos) + best_offset);
        if (read_pos + frame_size > input.size()) break;

        for (int i = 0; i < frame_size; ++i) {
            output[out_pos + i] += input[read_pos + i] * window[i];
        }

        in_pos += hop_in;
        out_pos += hop_out;
    }

    const float norm = static_cast<float>(hop_out) / static_cast<float>(frame_size);
    for (float& s : output) {
        s *= norm;
    }
}

// ---------------------------------------------------------------------------
// Per-track mixing
// ---------------------------------------------------------------------------
void AudioProcessor::mix_track(const TrackInstance& track, double startTimeSeconds, double durationSeconds, int sampleRate, std::vector<float>& mix_buffer) {
    // --- Validation ---
    if (!track.decoder) return;
    if (track.spec.volume <= 0.0f) return;
    if (track.spec.playback_speed <= 0.0f) return;
    if (track.spec.pitch_shift <= 0.0f) return;
    if (durationSeconds < 0.0) return;
    if (mix_buffer.empty()) return;
    
    // Check out_point for duration limit
    double track_source_duration = track.decoder->duration();
    if (track.spec.out_point_seconds > 0.0) {
        track_source_duration = std::min(track_source_duration, track.spec.out_point_seconds);
    }
    track_source_duration -= track.spec.in_point_seconds;

    const double track_start = track.spec.start_offset_seconds;
    const double track_end = track_start + (track_source_duration / std::max(0.01f, track.spec.playback_speed));
    const double request_end = startTimeSeconds + durationSeconds;

    if (track_end <= startTimeSeconds || track_start >= request_end) {
        return;
    }

    const float speed = track.spec.playback_speed;
    const float pitch = track.spec.pitch_shift;
    const bool pitch_correct = track.spec.pitch_correct;

    // --- Determine source read range ---
    const double time_within_track = std::max(0.0, startTimeSeconds - track_start);
    const double source_start = track.spec.in_point_seconds + time_within_track * speed;
    double source_duration = durationSeconds * speed;

    // Check trim_out bound
    if (track.spec.out_point_seconds > 0.0) {
        if (source_start >= track.spec.out_point_seconds) return;
        if (source_start + source_duration > track.spec.out_point_seconds) {
            source_duration = track.spec.out_point_seconds - source_start;
        }
    }

    std::vector<float> decoded = track.decoder->decode_range(source_start, source_duration + 0.1);
    if (decoded.empty()) return;

    const double out_offset_sec = std::max(0.0, track_start - startTimeSeconds);
    const std::size_t out_offset_samples = static_cast<std::size_t>(std::lround(out_offset_sec * sampleRate));

    const float volume = track.spec.volume;
    const float pan = std::clamp(track.spec.pan, -1.0f, 1.0f);
    const float angle = (pan + 1.0f) * (3.14159265f / 4.0f);
    const float left_gain = std::cos(angle) * volume;
    const float right_gain = std::sin(angle) * volume;

    const int kernel_radius = 4;
    const std::size_t target_samples = static_cast<std::size_t>(durationSeconds * sampleRate);

    std::vector<float> processed;

    if (pitch_correct && std::abs(speed - 1.0f) >= 0.01f) {
        std::vector<float> stretch_input = track.decoder->decode_range(source_start, source_duration / speed + 0.5);
        apply_time_stretch(stretch_input, speed, processed);

        if (std::abs(pitch - 1.0f) >= 0.01f) {
            std::vector<float> pitched;
            pitched.reserve(static_cast<std::size_t>(static_cast<float>(processed.size()) / pitch));
            for (std::size_t i = 0; i < pitched.capacity() / 2; ++i) {
                float src_pos = static_cast<float>(i) * pitch;
                int src_idx = static_cast<int>(std::floor(src_pos));
                float frac = src_pos - static_cast<float>(src_idx);
                for (int c = 0; c < 2; ++c) {
                    std::size_t s0 = static_cast<std::size_t>(src_idx) * 2 + c;
                    std::size_t s1 = s0 + 2;
                    float v0 = (s0 < processed.size()) ? processed[s0] : 0.0f;
                    float v1 = (s1 < processed.size()) ? processed[s1] : 0.0f;
                    pitched.push_back(v0 * (1.0f - frac) + v1 * frac);
                }
            }
            processed = std::move(pitched);
        }
    } else {
        const float resampling_ratio = speed * pitch * (static_cast<float>(kInternalSampleRate) / sampleRate);
        processed = std::move(decoded);

        std::vector<float> resampled;
        resampled.reserve(target_samples * 2);

        for (std::size_t i = 0; i < target_samples - out_offset_samples; ++i) {
            float src_pos = static_cast<float>(i) * resampling_ratio;
            int src_idx = static_cast<int>(std::floor(src_pos));
            float frac = src_pos - static_cast<float>(src_idx);

            float sum_l = 0.0f, sum_r = 0.0f, w_total = 0.0f;

            for (int k = -kernel_radius + 1; k <= kernel_radius; ++k) {
                int s_idx = src_idx + k;
                if (s_idx < 0 || s_idx >= static_cast<int>(processed.size() / 2)) continue;

                float x = static_cast<float>(k) - frac;
                float pi_x = 3.14159265f * x;
                float sinc = (std::abs(x) < 1e-6f) ? 1.0f : std::sin(pi_x) / pi_x;
                float lx = pi_x / kernel_radius;
                float lanczos = (std::abs(x) < static_cast<float>(kernel_radius)) ?
                                ((std::abs(x) < 1e-6f) ? 1.0f : std::sin(lx) / lx) : 0.0f;

                float weight = sinc * lanczos;
                sum_l += processed[s_idx * 2] * weight;
                sum_r += processed[s_idx * 2 + 1] * weight;
                w_total += weight;
            }

            if (std::abs(w_total) > 1e-6f) {
                float norm = 1.0f / w_total;
                resampled.push_back(sum_l * norm);
                resampled.push_back(sum_r * norm);
            } else {
                resampled.push_back(0.0f);
                resampled.push_back(0.0f);
            }
        }
        processed = std::move(resampled);
    }

    for (std::size_t i = 0; i < target_samples - out_offset_samples; ++i) {
        const std::size_t out_idx = (out_offset_samples + i) * 2;
        if (out_idx + 1 >= mix_buffer.size()) break;
        const std::size_t proc_idx = i * 2;
        if (proc_idx + 1 >= processed.size()) break;

        mix_buffer[out_idx]     += processed[proc_idx]     * left_gain;
        mix_buffer[out_idx + 1] += processed[proc_idx + 1] * right_gain;
    }

    const std::size_t track_samples = std::min(
        static_cast<std::size_t>(target_samples - out_offset_samples),
        processed.size() / 2);
    if (track_samples > 0) {
        m_graph.process_track(track.bus_id, mix_buffer.data() + out_offset_samples * 2, static_cast<int>(track_samples));
    }
}

void AudioProcessor::setup_track_effects(const TrackInstance& track) {
    if (track.spec.effects.empty()) return;
    
    AudioBus& bus = m_graph.get_or_create_bus(track.bus_id);
    const float sample_rate = static_cast<float>(kInternalSampleRate);
    
    for (const auto& effect : track.spec.effects) {
        if (effect.type == "fade_in") {
            auto node = std::make_shared<tachyon::audio::FadeNode>();
            double duration = effect.duration.value_or(1.0);
            float duration_samples = static_cast<float>(duration * sample_rate);
            node->set_fade_in(duration_samples, tachyon::audio::FadeNode::FadeType::Linear);
            if (effect.start_time.has_value()) {
                float start_samples = static_cast<float>(effect.start_time.value() * sample_rate);
                node->set_trim(start_samples, start_samples + duration_samples);
            }
            bus.add_node(node);
        } else if (effect.type == "fade_out") {
            auto node = std::make_shared<tachyon::audio::FadeNode>();
            double duration = effect.duration.value_or(1.0);
            float duration_samples = static_cast<float>(duration * sample_rate);
            node->set_fade_out(duration_samples, tachyon::audio::FadeNode::FadeType::Linear);
            if (effect.start_time.has_value()) {
                float start_samples = static_cast<float>(effect.start_time.value() * sample_rate);
                node->set_trim(start_samples, start_samples + duration_samples);
            }
            bus.add_node(node);
        } else if (effect.type == "gain") {
            float gain_db = effect.gain_db.value_or(0.0f);
            float gain_linear = std::pow(10.0f, gain_db / 20.0f);
            bus.add_node(std::make_shared<tachyon::audio::GainNode>(gain_linear));
        } else if (effect.type == "low_pass") {
            float cutoff = effect.cutoff_freq_hz.value_or(1000.0f);
            bus.add_node(std::make_shared<tachyon::audio::LowPassNode>(cutoff, sample_rate));
        } else if (effect.type == "high_pass") {
            float cutoff = effect.cutoff_freq_hz.value_or(1000.0f);
            bus.add_node(std::make_shared<tachyon::audio::HighPassNode>(cutoff, sample_rate));
        }
    }
}

} // namespace tachyon::audio
