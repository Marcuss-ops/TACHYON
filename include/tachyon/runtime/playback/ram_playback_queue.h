#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include <map>
#include <mutex>
#include <memory>

namespace tachyon::runtime {

/**
 * @brief Manages a sliding window of rendered frames in RAM for smooth playback.
 */
class RamPlaybackQueue {
public:
    struct Frame {
        std::int64_t frame_number;
        std::vector<std::uint8_t> data;
        int quality_tier;
    };

    explicit RamPlaybackQueue(std::size_t max_memory_bytes);

    /**
     * @brief Add a rendered frame to the RAM cache.
     */
    void push_frame(Frame frame);

    /**
     * @brief Get a frame from RAM if available.
     */
    [[nodiscard]] std::optional<Frame> get_frame(std::int64_t frame_number) const;

    /**
     * @brief Remove frames far from the current position.
     */
    void purge_outside_window(std::int64_t current_frame, std::int64_t window_size);

    /**
     * @brief Clear all cached frames.
     */
    void clear();

private:
    void enforce_memory_limit();

    std::size_t m_max_memory;
    mutable std::mutex m_mutex;
    std::map<std::int64_t, Frame> m_frames;
};

} // namespace tachyon::runtime