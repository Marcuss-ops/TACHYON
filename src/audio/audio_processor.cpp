#include "tachyon/audio/audio_processor.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

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
    }
}

void AudioProcessor::clear_tracks() {
    m_tracks.clear();
}

void AudioProcessor::process(double startTimeSeconds, double durationSeconds, int sampleRate, std::vector<float>& out_stereo_pcm) {
    const std::size_t target_samples = static_cast<std::size_t>(std::ceil(durationSeconds * sampleRate));
    out_stereo_pcm.assign(target_samples * 2, 0.0f);

    for (const auto& track : m_tracks) {
        mix_track(track, startTimeSeconds, durationSeconds, sampleRate, out_stereo_pcm);
    }

    m_graph.master().process(out_stereo_pcm.data(), static_cast<int>(target_samples));
}

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

void AudioProcessor::time_stretch(float speed, float pitch_ratio, const std::vector<float>& input, std::vector<float>& output) {
    if (!config_.enable_pitch_correct || std::abs(speed - 1.0f) < 0.01f) {
        output = input;
        return;
    }

    switch (config_.mode) {
        case AudioStretchMode::kWSOLA:
            apply_wsola(input, speed, output);
            break;
        case AudioStretchMode::kResample:
            fallback_resample(input, speed * pitch_ratio, output);
            break;
        default:
            // Fallback to simple resampling
            fallback_resample(input, speed, output);
            break;
    }
}

void AudioProcessor::reverse_audio(std::vector<float>& audio) {
    if (audio.size() < 2) return;
    std::reverse(audio.begin(), audio.end());
}

void AudioProcessor::apply_wsola(const std::vector<float>& input, float speed, std::vector<float>& output) {
    if (input.empty()) {
        output.clear();
        return;
    }

    const int frame_size = config_.frame_size;
    const int overlap = config_.overlap;
    const int step = frame_size - overlap;
    const int out_step = static_cast<int>(step / std::max(0.1f, speed));

    if (out_step <= 0) {
        output = input;
        return;
    }

    output.clear();
    std::vector<float> window(frame_size, 0.0f);
    for (int i = 0; i < frame_size; ++i) {
        float val = static_cast<float>(i) / (frame_size - 1);
        window[i] = 0.5f * (1.0f - std::cos(2.0f * 3.14159265f * val));
    }

    std::size_t in_pos = 0;
    std::size_t out_pos = 0;

    while (in_pos + frame_size <= input.size()) {
        if (out_pos + frame_size > output.size()) {
            output.resize(out_pos + frame_size, 0.0f);
        }

        // Simple OLA for now - WSOLA would search for best correlation offset
        for (int i = 0; i < frame_size; ++i) {
            output[out_pos + i] += input[in_pos + i] * window[i];
        }

        in_pos += step;
        out_pos += out_step;
    }
}

void AudioProcessor::fallback_resample(const std::vector<float>& input, float ratio, std::vector<float>& output) {
    if (input.empty()) {
        output.clear();
        return;
    }

    const std::size_t output_size = static_cast<std::size_t>(input.size() / std::max(0.1f, ratio));
    output.resize(output_size);

    for (std::size_t i = 0; i < output_size; ++i) {
        float src_pos = static_cast<float>(i) * ratio;
        int idx = static_cast<int>(src_pos);
        float frac = src_pos - idx;

        idx = std::clamp(idx, 0, static_cast<int>(input.size()) - 2);
        output[i] = input[idx] * (1.0f - frac) + input[idx + 1] * frac;
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

    const double source_start = std::max(0.0, startTimeSeconds - track_start) * speed;
    const double source_duration = durationSeconds * speed;

    std::vector<float> decoded = track.decoder->decode_range(source_start, source_duration + 0.1);
    if (decoded.empty()) return;

    // Apply pitch-correct time stretch if requested
    if (config_.enable_pitch_correct && std::abs(speed - 1.0f) > 0.01f) {
        std::vector<float> stretched;
        time_stretch(speed, 1.0f / speed, decoded, stretched);
        decoded = std::move(stretched);
    }

    const double out_offset_sec = std::max(0.0, track_start - startTimeSeconds);
    const std::size_t out_offset_samples = static_cast<std::size_t>(std::lround(out_offset_sec * sampleRate));

    const float volume = track.spec.volume;
    const float pan = std::clamp(track.spec.pan, -1.0f, 1.0f);
    const float angle = (pan + 1.0f) * (3.14159265f / 4.0f);
    const float left_gain = std::cos(angle) * volume;
    const float right_gain = std::sin(angle) * volume;

    const int kernel_radius = 4;
    const std::size_t target_samples = static_cast<std::size_t>(durationSeconds * sampleRate);

    const float resampling_ratio = speed * pitch * (static_cast<float>(kInternalSampleRate) / sampleRate);

    for (std::size_t i = 0; i < target_samples - out_offset_samples && i * 2 < decoded.size(); ++i) {
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
