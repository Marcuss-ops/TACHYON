#include "tachyon/runtime/playback/audio_preview_buffer.h"
#include <algorithm>
#include <cmath>

namespace tachyon::runtime {

AudioPreviewBuffer::AudioPreviewBuffer(int sample_rate, int channels)
    : m_sample_rate(sample_rate), m_channels(channels) {}

void AudioPreviewBuffer::append_samples(const float* data, std::size_t sample_count) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_buffer.insert(m_buffer.end(), data, data + sample_count);
}

std::vector<float> AudioPreviewBuffer::resample_for_scrub(
    double start_time, 
    double duration, 
    float speed) const {
    
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_buffer.empty() || std::abs(speed) < 0.01f) {
        return std::vector<float>(static_cast<std::size_t>(duration * m_sample_rate * m_channels), 0.0f);
    }

    const std::size_t output_sample_count = static_cast<std::size_t>(duration * m_sample_rate);
    std::vector<float> output;
    output.reserve(output_sample_count * m_channels);

    const double start_sample_idx = start_time * m_sample_rate;
    
    for (std::size_t i = 0; i < output_sample_count; ++i) {
        // Calculate the sample position in the source buffer based on speed
        double source_idx = start_sample_idx + (static_cast<double>(i) * speed);
        
        if (source_idx < 0 || source_idx >= (m_buffer.size() / m_channels) - 1) {
            for (int c = 0; c < m_channels; ++c) output.push_back(0.0f);
            continue;
        }

        // Linear interpolation between samples
        std::size_t idx_low = static_cast<std::size_t>(std::floor(source_idx));
        std::size_t idx_high = idx_low + 1;
        double frac = source_idx - idx_low;

        for (int c = 0; c < m_channels; ++c) {
            float s_low = m_buffer[idx_low * m_channels + c];
            float s_high = m_buffer[idx_high * m_channels + c];
            output.push_back(s_low * (1.0f - static_cast<float>(frac)) + s_high * static_cast<float>(frac));
        }
    }

    return output;
}

std::vector<float> AudioPreviewBuffer::time_stretch(
    double start_time,
    double duration,
    float speed) const {

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_buffer.empty() || std::abs(speed) < 0.01f || std::abs(speed - 1.0f) < 0.01f) {
        return resample_for_scrub(start_time, duration, 1.0f);
    }

    const std::size_t start_sample = static_cast<std::size_t>(start_time * m_sample_rate * m_channels);
    const std::size_t input_samples = static_cast<std::size_t>(duration * m_sample_rate * m_channels / speed);
    const std::size_t end_sample = std::min(start_sample + input_samples, m_buffer.size());
    const std::size_t actual_input = end_sample - start_sample;

    std::vector<float> output;
    ola_time_stretch(m_buffer.data() + start_sample, actual_input, speed, output, m_channels);
    return output;
}

void AudioPreviewBuffer::ola_time_stretch(
    const float* input,
    std::size_t input_samples,
    float speed,
    std::vector<float>& output,
    int channels) const {

    if (input_samples == 0 || channels <= 0) return;

    // OLA parameters: 20ms frame, 50% overlap
    const int frame_ms = 20;
    const int frame_size = (m_sample_rate * frame_ms) / 1000 * channels;
    const int hop_in = static_cast<int>(static_cast<float>(frame_size) / (2.0f * speed));
    const int hop_out = frame_size / 2;
    const int overlap = frame_size - hop_out;

    if (frame_size <= 0 || hop_in <= 0 || hop_out <= 0) return;

    const std::size_t total_input_frames = input_samples / static_cast<std::size_t>(channels);
    const std::size_t expected_output_frames = static_cast<std::size_t>(static_cast<float>(total_input_frames) * speed);
    output.resize(expected_output_frames * channels, 0.0f);

    // Hanning window
    std::vector<float> window(frame_size);
    for (int i = 0; i < frame_size; ++i) {
        window[i] = 0.5f - 0.5f * std::cos(2.0f * 3.14159265358979f * static_cast<float>(i) / static_cast<float>(frame_size));
    }

    std::size_t in_pos = 0;
    std::size_t out_pos = 0;

    while (in_pos + frame_size <= input_samples && out_pos + frame_size <= output.size()) {
        for (int i = 0; i < frame_size; ++i) {
            std::size_t src = in_pos + i;
            std::size_t dst = out_pos + i;
            if (dst < output.size() && src < input_samples) {
                output[dst] += input[src] * window[i];
            }
        }
        in_pos += hop_in;
        out_pos += hop_out;
    }

    // Normalize by overlap count
    const float norm = 1.0f / (static_cast<float>(frame_size) / static_cast<float>(hop_out));
    for (float& s : output) {
        s *= norm;
    }
}

void AudioPreviewBuffer::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_buffer.clear();
}

} // namespace tachyon::runtime
