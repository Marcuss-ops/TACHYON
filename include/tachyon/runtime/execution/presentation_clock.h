#pragma once

#include <atomic>
#include <cstdint>

namespace tachyon::runtime {

/**
 * @brief Master clock for the engine, driven by audio samples.
 * Ensures video frames stay in sync with audio playback.
 */
class PresentationClock {
public:
    PresentationClock(uint32_t sample_rate = 48000) 
        : m_sample_rate(sample_rate) {}

    /**
     * @brief Updates the clock with the number of samples processed by the audio device.
     */
    void update_from_audio(uint64_t samples) {
        if (m_is_playing) {
            m_samples_played.fetch_add(samples, std::memory_order_relaxed);
        }
    }

    [[nodiscard]] double get_current_time_seconds() const {
        return static_cast<double>(m_samples_played.load(std::memory_order_relaxed)) / m_sample_rate;
    }

    [[nodiscard]] uint32_t get_expected_video_frame(double fps) const {
        return static_cast<uint32_t>(get_current_time_seconds() * fps);
    }

    void start() { m_is_playing = true; }
    void stop()  { m_is_playing = false; }
    void reset() { m_samples_played.store(0); }
    void seek(double seconds) {
        m_samples_played.store(static_cast<uint64_t>(seconds * m_sample_rate));
    }

    [[nodiscard]] bool is_playing() const { return m_is_playing; }

private:
    std::atomic<uint64_t> m_samples_played{0};
    uint32_t m_sample_rate;
    std::atomic<bool> m_is_playing{false};
};

} // namespace tachyon::runtime
