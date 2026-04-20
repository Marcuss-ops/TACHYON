#pragma once

#include "tachyon/core/spec/scene_spec.h"

#include <cstdint>

namespace tachyon::timeline {

struct FrameSample {
    std::int64_t frame_number{0};
    double composition_time_seconds{0.0};
};

[[nodiscard]] double frame_to_seconds(const FrameRate& frame_rate, std::int64_t frame_number) noexcept;
[[nodiscard]] FrameSample sample_frame(const FrameRate& frame_rate, std::int64_t frame_number) noexcept;
[[nodiscard]] double local_time_from_composition(double composition_time_seconds, double start_time_seconds) noexcept;

} // namespace tachyon::timeline
