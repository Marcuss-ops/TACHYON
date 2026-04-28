#include "tachyon/tracker/algorithms/optical_flow_math.h"
#include "tachyon/core/math/algebra/vector2.h"
#include <cmath>
#include <algorithm>

namespace tachyon::tracker {

bool OpticalFlowMath::compute_gradients(
    const std::vector<float>& prev,
    const std::vector<float>& curr,
    int width, int height,
    int cx, int cy,
    int window_half,
    Gradients& out) {

    out = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    if (window_half <= 0) return false;

    auto clamp = [width, height](int x, int y) -> size_t {
        x = std::max(0, std::min(x, width - 1));
        y = std::max(0, std::min(y, height - 1));
        return static_cast<size_t>(y * width + x);
    };

    for (int dy = -window_half; dy <= window_half; ++dy) {
        for (int dx = -window_half; dx <= window_half; ++dx) {
            int x = cx + dx;
            int y = cy + dy;
            if (x < 0 || x >= width || y < 0 || y >= height) continue;

            // Spatial gradients (central difference)
            float gx = (prev[clamp(x + 1, y)] - prev[clamp(x - 1, y)]) * 0.5f;
            float gy = (prev[clamp(x, y + 1)] - prev[clamp(x, y - 1)]) * 0.5f;

            // Temporal gradient (curr - prev)
            float It = curr[clamp(x, y)] - prev[clamp(x, y)];

            out.Ixx += gx * gx;
            out.Iyy += gy * gy;
            out.Ixy += gx * gy;
            out.Ixt += gx * It;
            out.Iyt += gy * It;
        }
    }

    // Check if we have enough data for a valid solution
    float det = out.Ixx * out.Iyy - out.Ixy * out.Ixy;
    return std::abs(det) > 1e-8f;
}

math::Vector2 OpticalFlowMath::solve_lk(
    const Gradients& g,
    float eigenvalue_threshold,
    float max_flow_magnitude,
    bool& was_clamped) {

    was_clamped = false;
    float det = g.Ixx * g.Iyy - g.Ixy * g.Ixy;
    if (std::abs(det) < 1e-8f) return math::Vector2::zero();

    // Check eigenvalue threshold (minimum eigenvalue of structure tensor)
    float trace = g.Ixx + g.Iyy;
    float disc = trace * trace - 4 * det;
    if (disc < 0) return math::Vector2::zero();
    float min_eig = (trace - std::sqrt(disc)) * 0.5f;
    if (min_eig < eigenvalue_threshold) return math::Vector2::zero();

    // Solve [Ixx Ixy; Ixy Iyy] * [u; v] = [-Ixt; -Iyt]
    float inv_det = 1.0f / det;
    math::Vector2 flow(
        (g.Ixy * g.Iyt - g.Iyy * g.Ixt) * inv_det,
        (g.Ixy * g.Ixt - g.Ixx * g.Iyt) * inv_det
    );

    // Clamp flow magnitude
    float mag = flow.length();
    if (mag > max_flow_magnitude) {
        flow = flow * (max_flow_magnitude / mag);
        was_clamped = true;
    }

    return flow;
}

float OpticalFlowMath::calculate_confidence(
    float Ixx, float Ixy, float Iyy,
    const math::Vector2& flow,
    bool was_clamped,
    float max_flow_magnitude) {

    // Confidence based on structure tensor eigenvalue and flow consistency
    float trace = Ixx + Iyy;
    float det = Ixx * Iyy - Ixy * Ixy;
    if (trace < 1e-8f) return 0.0f;

    float min_eig = (trace - std::sqrt(trace * trace - 4 * det)) * 0.5f;
    float conf = std::clamp(min_eig / 100.0f, 0.0f, 1.0f); // Normalize eigenvalue

    // Penalize clamped flows
    if (was_clamped) conf *= 0.5f;

    // Penalize large flows
    float mag = flow.length();
    if (mag > max_flow_magnitude * 0.5f) {
        conf *= 1.0f - (mag / max_flow_magnitude);
    }

    return std::max(0.0f, conf);
}

void OpticalFlowMath::downsample(const std::vector<float>& src, int src_w, int src_h,
                                std::vector<float>& dst, int& dst_w, int& dst_h) {
    dst_w = src_w / 2;
    dst_h = src_h / 2;
    if (dst_w < 1) dst_w = 1;
    if (dst_h < 1) dst_h = 1;

    dst.resize(static_cast<size_t>(dst_w * dst_h));

    for (int y = 0; y < dst_h; ++y) {
        for (int x = 0; x < dst_w; ++x) {
            int src_x = x * 2;
            int src_y = y * 2;

            float sum = 0.0f;
            int count = 0;
            for (int dy = 0; dy < 2 && src_y + dy < src_h; ++dy) {
                for (int dx = 0; dx < 2 && src_x + dx < src_w; ++dx) {
                    size_t idx = static_cast<size_t>((src_y + dy) * src_w + (src_x + dx));
                    if (idx < src.size()) {
                        sum += src[idx];
                        count++;
                    }
                }
            }
            size_t dst_idx = static_cast<size_t>(y * dst_w + x);
            dst[dst_idx] = count > 0 ? sum / count : 0.0f;
        }
    }
}

void OpticalFlowMath::build_pyramid(const std::vector<float>& frame, int width, int height,
                                    int pyramid_levels,
                                    std::vector<std::vector<float>>& pyramid,
                                    std::vector<int>& widths, std::vector<int>& heights) {
    pyramid.clear();
    widths.clear();
    heights.clear();

    pyramid.push_back(frame);
    widths.push_back(width);
    heights.push_back(height);

    for (int level = 1; level < pyramid_levels; ++level) {
        int prev_w = widths.back();
        int prev_h = heights.back();
        std::vector<float> downsampled;
        int new_w, new_h;
        downsample(pyramid.back(), prev_w, prev_h, downsampled, new_w, new_h);
        pyramid.push_back(std::move(downsampled));
        widths.push_back(new_w);
        heights.push_back(new_h);
    }
}

float OpticalFlowMath::compute_forward_backward_error(const math::Vector2& fw, const math::Vector2& bw) {
    math::Vector2 inconsistency = fw + bw;
    return inconsistency.length();
}

float OpticalFlowMath::apply_occlusion_confidence_penalty(float current_confidence, float error) {
    return current_confidence * std::exp(-error * 0.5f);
}

math::Vector2 OpticalFlowMath::smooth_flow(const math::Vector2& curr, const math::Vector2& prev, float alpha) {
    return curr * alpha + prev * (1.0f - alpha);
}

float OpticalFlowMath::smooth_confidence(float curr, float prev, float alpha) {
    return curr * alpha + prev * (1.0f - alpha);
}

} // namespace tachyon::tracker
