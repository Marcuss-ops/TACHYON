#include "tachyon/audio/audio_processor.h"
#include <algorithm>
#include <cmath>

namespace tachyon::audio {

AudioProcessor::AudioProcessor() = default;
AudioProcessor::~AudioProcessor() = default;

void AudioProcessor::add_track(std::shared_ptr<AudioDecoder> decoder, const spec::AudioTrackSpec& spec, const std::string& bus_id) {
    if (decoder) {
        m_tracks.push_back({std::move(decoder), spec, bus_id});
    }
}

void AudioProcessor::clear_tracks() {
    m_tracks.clear();
}

void AudioProcessor::process(double startTimeSeconds, double durationSeconds, int sampleRate, std::vector<float>& out_stereo_pcm) {
    const std::size_t target_samples = static_cast<std::size_t>(std::ceil(durationSeconds * sampleRate));
    out_stereo_pcm.assign(target_samples * 2, 0.0f);

    std::vector<float> track_buffer;
    for (const auto& track : m_tracks) {
        mix_track(track, startTimeSeconds, durationSeconds, sampleRate, out_stereo_pcm);
    }

    // Post-processing master bus
    m_graph.master().process(out_stereo_pcm.data(), static_cast<int>(target_samples));
}

void AudioProcessor::apply_time_stretch(const std::vector<float>& input, float speed, std::vector<float>& output) {
    if (std::abs(speed - 1.0f) < 0.01f) {
        output = input;
        return;
    }

    const int channels = kInternalSampleRate == 48000 ? 2 : 2; // Assuming stereo
    const int frame_size = 1024;
    const int overlap = 512;
    const int step = frame_size - overlap;
    const int out_step = static_cast<int>(step / speed);

    if (out_step <= 0) return;

    output.clear();
    std::vector<float> window(frame_size, 0.0f);
    for (int i = 0; i < frame_size; ++i) {
        window[i] = 0.5f * (1.0f - std::cos(2.0f * 3.14159265f * i / (frame_size - 1)));
    }

    std::size_t in_pos = 0;
    std::size_t out_pos = 0;
    
    // This is a VERY simplified OLA (Overlap-Add) for now.
    // Real WSOLA would search for the best correlation offset.
    while (in_pos + frame_size * channels < input.size()) {
        if (out_pos + frame_size * channels > output.size()) {
            output.resize(out_pos + frame_size * channels, 0.0f);
        }

        for (int i = 0; i < frame_size; ++i) {
            for (int ch = 0; ch < channels; ++ch) {
                output[out_pos + i * channels + ch] += input[in_pos + i * channels + ch] * window[i];
            }
        }

        in_pos += step * channels;
        out_pos += out_step * channels;
    }
}

void AudioProcessor::mix_track(const TrackInstance& track, double startTimeSeconds, double durationSeconds, int sampleRate, std::vector<float>& mix_buffer) {
    if (track.spec.volume <= 0.0f) return;

    const double track_start = track.spec.start_offset_seconds;
    const double track_end = track_start + (track.decoder->duration() / std::max(0.01f, track.spec.playback_speed));
    const double request_end = startTimeSeconds + durationSeconds;

    if (track_end <= startTimeSeconds || track_start >= request_end) {
        return;
    }

    const float speed = std::max(0.01f, track.spec.playback_speed);
    const float pitch = std::max(0.01f, track.spec.pitch_shift);

    // If we want time-stretch, we adjust source_duration
    const double source_start = std::max(0.0, startTimeSeconds - track_start) * speed;
    const double source_duration = durationSeconds * speed;

    std::vector<float> decoded = track.decoder->decode_range(source_start, source_duration + 0.1);
    if (decoded.empty()) return;

    // TODO: Apply WSOLA here if pitch preservation is requested
    // For now, we continue with high-quality resampling which changes pitch with speed
    // unless speed == 1.0 and pitch != 1.0.

    const double out_offset_sec = std::max(0.0, track_start - startTimeSeconds);
    const std::size_t out_offset_samples = static_cast<std::size_t>(std::lround(out_offset_sec * sampleRate));

    const float volume = track.spec.volume;
    const float pan = std::clamp(track.spec.pan, -1.0f, 1.0f);
    const float angle = (pan + 1.0f) * (3.14159265f / 4.0f);
    const float left_gain = std::cos(angle) * volume;
    const float right_gain = std::sin(angle) * volume;

    const int kernel_radius = 4;
    const std::size_t target_samples = static_cast<std::size_t>(durationSeconds * sampleRate);

    // Resampling ratio:
    // If we want to preserve pitch (pitch_shift = 1.0) while changing speed:
    // We need time-stretch (WSOLA).
    // If we want to change pitch while keeping speed:
    // We need resampling.
    
    // For now, our resampler handles pitch change.
    // To 'correct' pitch when speed changes, we would set pitch_shift = 1.0/speed.
    const float resampling_ratio = speed * pitch * (static_cast<float>(kInternalSampleRate) / sampleRate);

    for (std::size_t i = 0; i < target_samples - out_offset_samples; ++i) {
        float src_pos = static_cast<float>(i) * resampling_ratio;
        int src_idx = static_cast<int>(std::floor(src_pos));
        float frac = src_pos - static_cast<float>(src_idx);

        float sum_l = 0.0f, sum_r = 0.0f, w_total = 0.0f;

        for (int k = -kernel_radius + 1; k <= kernel_radius; ++k) {
            int s_idx = src_idx + k;
            if (s_idx < 0 || s_idx >= static_cast<int>(decoded.size() / 2)) continue;

            float x = static_cast<float>(k) - frac;
            float pi_x = 3.14159265f * x;
            float sinc = (std::abs(x) < 1e-6f) ? 1.0f : std::sin(pi_x) / pi_x;
            float lx = pi_x / kernel_radius;
            float lanczos = (std::abs(x) < static_cast<float>(kernel_radius)) ? 
                            ((std::abs(x) < 1e-6f) ? 1.0f : std::sin(lx) / lx) : 0.0f;
            
            float weight = sinc * lanczos;
            sum_l += decoded[s_idx * 2] * weight;
            sum_r += decoded[s_idx * 2 + 1] * weight;
            w_total += weight;
        }

        if (std::abs(w_total) > 1e-6f) {
            const std::size_t out_idx = (out_offset_samples + i) * 2;
            if (out_idx + 1 < mix_buffer.size()) {
                float norm = 1.0f / w_total;
                mix_buffer[out_idx] += sum_l * left_gain * norm;
                mix_buffer[out_idx + 1] += sum_r * right_gain * norm;
            }
        }
    }

    m_graph.process_track(track.bus_id, mix_buffer.data() + out_offset_samples * 2, static_cast<int>(target_samples - out_offset_samples));
}

} // namespace tachyon::audio
