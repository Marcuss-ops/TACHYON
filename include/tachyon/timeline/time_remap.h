#pragma once
#include "tachyon/core/spec/schema/contracts/shared_contracts.h"
#include <vector>
#include <utility>
#include <string>

namespace tachyon::timeline {

using TimeRemapMode = spec::TimeRemapMode;
using FrameBlendMode = spec::FrameBlendMode;
using TimeRemapCurve = spec::TimeRemapCurve;

struct FrameBlendResult {
    double source_time_a;
    double source_time_b;
    float blend_factor; // 0.0 = only A, 1.0 = only B
};

struct FrameBlendParams {
    FrameBlendMode mode{FrameBlendMode::Linear};
    float blend_strength{1.0f}; // 0.0 = no blend, 1.0 = full blend
};

/**
 * @brief Evaluates the source time for a given destination time on the curve.
 */
[[nodiscard]] float evaluate_source_time(const TimeRemapCurve& curve, float dest_time);

/**
 * @brief Evaluates the frame blend parameters for a given destination time.
 */
[[nodiscard]] FrameBlendResult evaluate_frame_blend(
    const TimeRemapCurve& curve, 
    float dest_time, 
    float frame_duration);

} // namespace tachyon::timeline
