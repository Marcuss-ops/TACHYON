#pragma once

#include "tachyon/tracker/algorithms/optical_flow.h"
#include "tachyon/tracker/algorithms/optical_flow_helpers.h"
#include <vector>

namespace tachyon::tracker {

class OpticalFlowMath {
public:
    struct Gradients {
        float Ixx, Ixy, Iyy, Ixt, Iyt;
    };

    static bool compute_gradients(
        const std::vector<float>& prev,
        const std::vector<float>& curr,
        int width, int height,
        int cx, int cy,
        int window_half,
        Gradients& out);

    static float calculate_confidence(
        float Ixx, float Ixy, float Iyy,
        const math::Vector2& flow,
        bool was_clamped,
        float max_flow_magnitude);

    static math::Vector2 solve_lk(
        const Gradients& g,
        float eigenvalue_threshold,
        float max_flow_magnitude,
        bool& was_clamped);

    // Image pyramid construction
    static void downsample(const std::vector<float>& src, int src_w, int src_h,
                           std::vector<float>& dst, int& dst_w, int& dst_h);
    static void build_pyramid(const std::vector<float>& frame, int width, int height,
                               int pyramid_levels,
                               std::vector<std::vector<float>>& pyramid,
                               std::vector<int>& widths, std::vector<int>& heights);

    // Occlusion detection math
    static float compute_forward_backward_error(const math::Vector2& fw, const math::Vector2& bw);
    static float apply_occlusion_confidence_penalty(float current_confidence, float error);

    // Temporal smoothing math
    static math::Vector2 smooth_flow(const math::Vector2& curr, const math::Vector2& prev, float alpha);
    static float smooth_confidence(float curr, float prev, float alpha);
};

} // namespace tachyon::tracker
