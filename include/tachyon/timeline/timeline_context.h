#pragma once

#include <cstdint>
#include <optional>

namespace tachyon::timeline {

/**
 * @brief Shared, immutable representation of evaluation time.
 * 
 * This is the single source of truth for time across:
 * - evaluate_composition_state(...)
 * - evaluate_frame_state(...)
 * - render_plan execution
 * - motion blur sampling
 * - previous-frame evaluation
 */
struct TimelineContext {
    std::int64_t frame_number{0};
    double frame_time_seconds{0.0};
    double frame_duration_seconds{0.0};

    // Subframe support
    bool is_subframe{false};
    std::optional<std::int64_t> main_frame_number;
    std::optional<double> main_frame_time_seconds;
    std::size_t subframe_index{0};
    std::size_t subframe_count{1};

    // Motion blur shutter window
    std::optional<double> shutter_start_seconds;
    std::optional<double> shutter_end_seconds;

    // Helper: compute previous frame time
    [[nodiscard]] double previous_frame_time_seconds() const noexcept {
        return frame_time_seconds - frame_duration_seconds;
    }

    // Helper: check if this is a subframe
    [[nodiscard]] bool is_subframe_sample() const noexcept {
        return is_subframe;
    }
};

} // namespace tachyon::timeline
