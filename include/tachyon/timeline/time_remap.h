#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/spec/schema/contracts/shared_contracts.h"
#include "tachyon/tracker/optical_flow.h"
#include <vector>
#include <utility>
#include <string>
#include <memory>

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
[[nodiscard]] TACHYON_API float evaluate_source_time(const TimeRemapCurve& curve, float dest_time);

/**
 * @brief Evaluates the frame blend parameters for a given destination time.
 */
[[nodiscard]] TACHYON_API FrameBlendResult evaluate_frame_blend(
    const TimeRemapCurve& curve, 
    float dest_time, 
    float frame_duration);

/**
 * @brief Evaluator for time remapping with optical flow support.
 */
class TACHYON_API TimeRemapEvaluator {
public:
    struct Config {
        float optical_flow_confidence_threshold;
        bool enable_pixel_motion;
        bool enable_optical_flow_warping;
        Config() : optical_flow_confidence_threshold(0.5f), enable_pixel_motion(true), enable_optical_flow_warping(true) {}
    };

    explicit TimeRemapEvaluator(const Config& config = Config());
    
    /**
     * @brief Evaluate source time with fallback based on confidence.
     */
    float evaluate(const TimeRemapCurve& curve, float dest_time, float frame_duration) const;
    
    /**
     * @brief Get frame blend result with optical flow warping if enabled.
     */
    FrameBlendResult evaluate_with_flow(
        const TimeRemapCurve& curve,
        float dest_time,
        float frame_duration,
        const tracker::OpticalFlowResult* flow_result = nullptr) const;
    
    /**
     * @brief Warp pixels using optical flow for pixel motion blending.
     */
    std::vector<float> warp_frame(
        const std::vector<float>& frame,
        int width, int height, int channels,
        const tracker::OpticalFlowResult& flow,
        float warp_factor = 1.0f) const;

private:
    Config config_;
};

} // namespace tachyon::timeline
