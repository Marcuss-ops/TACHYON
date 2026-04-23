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

void AudioPreviewBuffer::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_buffer.clear();
}

} // namespace tachyon::runtime
