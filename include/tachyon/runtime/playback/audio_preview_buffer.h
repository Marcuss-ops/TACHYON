#pragma once

#include <vector>
#include <mutex>
#include <cstdint>

namespace tachyon::runtime {

/**
 * @brief Buffer for synchronized audio playback and scrubbing.
 * 
 * Supports resampling for "scrub" effect (pitch-shifted or time-stretched audio).
 */
class AudioPreviewBuffer {
public:
    explicit AudioPreviewBuffer(int sample_rate = 48000, int channels = 2);

    /**
     * @brief Append decoded PCM data to the buffer.
     */
    void append_samples(const float* data, std::size_t sample_count);

    /**
     * @brief Get resampled audio for a specific scrub speed.
     * 
     * @param start_time Position in seconds.
     * @param duration Duration of the segment to retrieve.
     * @param speed Playback speed (1.0 = normal, 2.0 = double, -1.0 = reverse).
     * @return Interleaved PCM samples.
     */
    [[nodiscard]] std::vector<float> resample_for_scrub(
        double start_time, 
        double duration, 
        float speed) const;

    /**
     * @brief Clear the buffer.
     */
    void clear();

private:
    int m_sample_rate;
    int m_channels;
    mutable std::mutex m_mutex;
    std::vector<float> m_buffer; // Interleaved samples
};

} // namespace tachyon::runtime
