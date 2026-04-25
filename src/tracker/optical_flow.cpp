#include "tachyon/tracker/optical_flow.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <numeric>
#include <vector>

namespace tachyon::tracker {

// ============ OpticalFlowResult ============

const OpticalFlowVector* OpticalFlowResult::get_flow_at(float nx, float ny) const {
    int x = static_cast<int>(nx * width);
    int y = static_cast<int>(ny * height);
    x = std::max(0, std::min(x, width - 1));
    y = std::max(0, std::min(y, height - 1));
    size_t idx = static_cast<size_t>(y * width + x);
    if (idx < vectors.size()) return &vectors[idx];
    return nullptr;
}

const OpticalFlowVector* OpticalFlowResult::get_flow_at_pixel(int x, int y) const {
    x = std::max(0, std::min(x, width - 1));
    y = std::max(0, std::min(y, height - 1));
    size_t idx = static_cast<size_t>(y * width + x);
    if (idx < vectors.size()) return &vectors[idx];
    return nullptr;
}

float OpticalFlowResult::average_confidence() const {
    if (vectors.empty()) return 0.0f;
    float sum = 0.0f;
    int count = 0;
    for (const auto& v : vectors) {
        if (v.valid) {
            sum += v.confidence;
            count++;
        }
    }
    return count > 0 ? sum / count : 0.0f;
}

int OpticalFlowResult::valid_count() const {
    return static_cast<int>(std::count_if(vectors.begin(), vectors.end(),
        [](const auto& v) { return v.valid; }));
}

// ============ OpticalFlowConsumer ============

OpticalFlowConsumer::OpticalFlowConsumer(const Config& config) : config_(config) {}

OpticalFlowConsumer::ConsumerResult OpticalFlowConsumer::consume(const OpticalFlowVector& vec) const {
    ConsumerResult result;

    // Low confidence or explicitly degraded -> explicit fallback
    if (!vec.valid || vec.confidence < config_.low_confidence_threshold) {
        result.action = config_.enable_hold_for_low_conf
                        ? Action::kHold
                        : Action::kZeroMotion;
        result.flow = math::Vector2::zero();
        result.weight = 0.0f;
    }
    // Medium confidence -> blend with reduced weight
    else if (vec.confidence < config_.high_confidence_threshold) {
        result.action = Action::kBlend;
        result.flow = vec.flow;
        result.weight = config_.blend_factor * vec.confidence;
    }
    // High confidence -> use flow directly
    else {
        result.action = Action::kFlow;
        result.flow = vec.flow;
        result.weight = 1.0f;
    }

    return result;
}

OpticalFlowResult OpticalFlowConsumer::process(const OpticalFlowResult& flow,
                                                const OpticalFlowResult& prev_flow) const {
    OpticalFlowResult result;
    result.width = flow.width;
    result.height = flow.height;
    result.vectors.resize(flow.vectors.size());

    const bool have_prev = !prev_flow.vectors.empty()
                        && prev_flow.vectors.size() == flow.vectors.size()
                        && prev_flow.width == flow.width
                        && prev_flow.height == flow.height;

    for (size_t i = 0; i < flow.vectors.size(); ++i) {
        auto cr = consume(flow.vectors[i]);

        if (cr.action == Action::kHold) {
            // HOLD: use previous frame's flow explicitly
            if (have_prev && prev_flow.vectors[i].valid) {
                // Carry forward the held flow but mark confidence as 0
                // so downstream knows it was not freshly computed.
                result.vectors[i] = OpticalFlowVector(
                    prev_flow.vectors[i].flow,
                    0.0f,      // held flow has no fresh confidence
                    false,     // not a valid new measurement
                    static_cast<uint8_t>(std::min(255, static_cast<int>(prev_flow.vectors[i].age) + 1)),
                    prev_flow.vectors[i].source_level
                );
            } else {
                // No previous flow available -> zero motion
                result.vectors[i] = OpticalFlowVector(
                    math::Vector2::zero(), 0.0f, false, 0, -1);
            }
        }
        else if (cr.action == Action::kZeroMotion) {
            // ZERO MOTION: explicit zero
            result.vectors[i] = OpticalFlowVector(
                math::Vector2::zero(), 0.0f, false, 0, -1);
        }
        else if (cr.action == Action::kBlend) {
            // BLEND: weighted flow from current (already degraded by calculator if bad)
            math::Vector2 final_flow = cr.flow * cr.weight;
            result.vectors[i] = OpticalFlowVector(
                final_flow,
                cr.weight,
                cr.weight > 0.01f,
                flow.vectors[i].age,
                flow.vectors[i].source_level
            );
        }
        else { // kFlow
            // FLOW: use directly
            result.vectors[i] = OpticalFlowVector(
                cr.flow,
                1.0f,
                true,
                flow.vectors[i].age,
                flow.vectors[i].source_level
            );
        }
    }

    return result;
}

// ============ OpticalFlowCalculator - Helpers ============

struct Matrix2x2 {
    float m[2][2];

    float det() const {
        return m[0][0] * m[1][1] - m[0][1] * m[1][0];
    }

    Matrix2x2 inverse() const {
        float d = det();
        Matrix2x2 inv{};
        if (std::abs(d) > 1e-6f) {
            inv.m[0][0] = m[1][1] / d;
            inv.m[0][1] = -m[0][1] / d;
            inv.m[1][0] = -m[1][0] / d;
            inv.m[1][1] = m[0][0] / d;
        }
        return inv;
    }
};

struct TranslationEstimate {
    bool found{false};
    int dx{0};
    int dy{0};
    float confidence{0.0f};
};

TranslationEstimate estimate_translation(
    const std::vector<float>& prev,
    const std::vector<float>& curr,
    int width,
    int height,
    int max_shift) {

    TranslationEstimate estimate;
    if (width <= 0 || height <= 0) {
        return estimate;
    }

    const int shift_limit = std::max(0, std::min(max_shift, std::min(width - 1, height - 1)));
    if (shift_limit == 0) {
        return estimate;
    }

    auto sample = [&](const std::vector<float>& frame, int x, int y) -> float {
        return frame[static_cast<std::size_t>(y * width + x)];
    };

    float baseline_mse = std::numeric_limits<float>::infinity();
    float best_mse = std::numeric_limits<float>::infinity();
    int best_dx = 0;
    int best_dy = 0;

    for (int dy = -shift_limit; dy <= shift_limit; ++dy) {
        for (int dx = -shift_limit; dx <= shift_limit; ++dx) {
            float error = 0.0f;
            int samples = 0;

            for (int y = 0; y < height; ++y) {
                const int py = y - dy;
                if (py < 0 || py >= height) {
                    continue;
                }
                for (int x = 0; x < width; ++x) {
                    const int px = x - dx;
                    if (px < 0 || px >= width) {
                        continue;
                    }

                    const float delta = sample(prev, px, py) - sample(curr, x, y);
                    error += delta * delta;
                    ++samples;
                }
            }

            if (samples == 0) {
                continue;
            }

            const float mse = error / static_cast<float>(samples);
            if (dx == 0 && dy == 0) {
                baseline_mse = mse;
            }

            if (mse < best_mse) {
                best_mse = mse;
                best_dx = dx;
                best_dy = dy;
            }
        }
    }

    if (!std::isfinite(baseline_mse) || !std::isfinite(best_mse)) {
        return estimate;
    }

    const float improvement = (baseline_mse - best_mse) / std::max(1e-6f, baseline_mse);
    if (improvement < 0.05f) {
        return estimate;
    }

    estimate.found = true;
    estimate.dx = best_dx;
    estimate.dy = best_dy;
    estimate.confidence = std::clamp(improvement, 0.0f, 1.0f);
    if (std::abs(best_dx) == shift_limit || std::abs(best_dy) == shift_limit) {
        estimate.confidence = std::min(estimate.confidence, 0.25f);
    } else {
        estimate.confidence = std::max(estimate.confidence, 0.65f);
    }

    return estimate;
}

void apply_translation_estimate(
    OpticalFlowResult& result,
    const TranslationEstimate& estimate) {

    if (!estimate.found) {
        return;
    }

    for (auto& vec : result.vectors) {
        vec.flow = math::Vector2(static_cast<float>(estimate.dx), static_cast<float>(estimate.dy));
        vec.confidence = estimate.confidence;
        vec.valid = estimate.confidence > 0.0f;
        vec.age = 0;
        vec.source_level = -1;
    }
}

OpticalFlowCalculator::OpticalFlowCalculator(const Config& config) : config_(config) {}

void OpticalFlowCalculator::downsample(const std::vector<float>& src, int src_w, int src_h,
                                        std::vector<float>& dst, int& dst_w, int& dst_h) const {
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

void OpticalFlowCalculator::build_pyramid(const std::vector<float>& frame, int width, int height,
                                           std::vector<std::vector<float>>& pyramid,
                                           std::vector<int>& widths,
                                           std::vector<int>& heights) const {
    pyramid.clear();
    widths.clear();
    heights.clear();

    pyramid.push_back(frame);
    widths.push_back(width);
    heights.push_back(height);

    for (int level = 1; level < config_.pyramid_levels; ++level) {
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

bool OpticalFlowCalculator::compute_gradient_matrices(const std::vector<float>& prev, const std::vector<float>& curr,
                                                       int width, int height, int cx, int cy, int window_half,
                                                       float& Ixx, float& Ixy, float& Iyy, float& Ixt, float& Iyt) const {
    Ixx = Ixy = Iyy = Ixt = Iyt = 0.0f;

    for (int dy = -window_half; dy <= window_half; ++dy) {
        for (int dx = -window_half; dx <= window_half; ++dx) {
            int x = cx + dx;
            int y = cy + dy;

            if (x < 1 || x >= width - 1 || y < 1 || y >= height - 1) continue;

            size_t idx   = static_cast<size_t>(y * width + x);
            size_t idx_r = static_cast<size_t>(y * width + (x + 1));
            size_t idx_l = static_cast<size_t>(y * width + (x - 1));
            size_t idx_d = static_cast<size_t>((y + 1) * width + x);
            size_t idx_u = static_cast<size_t>((y - 1) * width + x);

            if (idx >= prev.size() || idx >= curr.size()) continue;

            float Ix = (prev[idx_r] - prev[idx_l] + curr[idx_r] - curr[idx_l]) * 0.25f;
            float Iy = (prev[idx_d] - prev[idx_u] + curr[idx_d] - curr[idx_u]) * 0.25f;
            float It = curr[idx] - prev[idx];

            Ixx += Ix * Ix;
            Ixy += Ix * Iy;
            Iyy += Iy * Iy;
            Ixt += Ix * It;
            Iyt += Iy * It;
        }
    }

    return (Ixx + Iyy) > 1e-6f;
}

void OpticalFlowCalculator::compute_lk(const std::vector<float>& prev, const std::vector<float>& curr,
                                        int width, int height, OpticalFlowResult& result, int level) const {
    int window_half = config_.window_size / 2;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = static_cast<size_t>(y * width + x);
            if (idx >= result.vectors.size()) continue;

            float Ixx, Ixy, Iyy, Ixt, Iyt;
            if (!compute_gradient_matrices(prev, curr, width, height, x, y, window_half,
                                           Ixx, Ixy, Iyy, Ixt, Iyt)) {
                result.vectors[idx].valid = false;
                result.vectors[idx].confidence = 0.0f;
                result.vectors[idx].flow = math::Vector2::zero();
                result.vectors[idx].source_level = static_cast<int8_t>(level);
                continue;
            }

            Matrix2x2 M{{Ixx, Ixy, Ixy, Iyy}};
            float det = M.det();

            math::Vector2 flow;
            bool was_clamped = false;

            if (std::abs(det) > config_.eigenvalue_threshold) {
                Matrix2x2 Minv = M.inverse();
                math::Vector2 neg_b{-Ixt, -Iyt};
                flow = math::Vector2(
                    Minv.m[0][0] * neg_b.x + Minv.m[0][1] * neg_b.y,
                    Minv.m[1][0] * neg_b.x + Minv.m[1][1] * neg_b.y
                );

                // Clamp flow to reasonable range
                float max_flow = config_.max_flow_magnitude;
                if (std::abs(flow.x) > max_flow) {
                    flow.x = (flow.x > 0) ? max_flow : -max_flow;
                    was_clamped = true;
                }
                if (std::abs(flow.y) > max_flow) {
                    flow.y = (flow.y > 0) ? max_flow : -max_flow;
                    was_clamped = true;
                }

                float conf = calculate_confidence(Ixx, Ixy, Iyy, flow, was_clamped);
                bool valid = conf > config_.confidence_threshold;

                result.vectors[idx].flow = flow;
                result.vectors[idx].confidence = conf;
                result.vectors[idx].valid = valid;
                result.vectors[idx].source_level = static_cast<int8_t>(level);

                // EXPLICIT DEGRADATION inside LK:
                // If confidence is too low, zero the flow immediately.
                if (!valid) {
                    result.vectors[idx].flow = math::Vector2::zero();
                }
            } else {
                result.vectors[idx].valid = false;
                result.vectors[idx].confidence = 0.0f;
                result.vectors[idx].flow = math::Vector2::zero();
                result.vectors[idx].source_level = static_cast<int8_t>(level);
            }
        }
    }
}

float OpticalFlowCalculator::calculate_confidence(float Ixx, float Ixy, float Iyy,
                                                   const math::Vector2& flow,
                                                   bool was_clamped) const {
    // 1. Gradient response: use min eigenvalue relative to max eigenvalue
    float trace = Ixx + Iyy;
    float det = Ixx * Iyy - Ixy * Ixy;

    if (trace < 1e-6f) return 0.0f;

    float sqrt_term = std::sqrt(std::max(0.0f, trace * trace - 4.0f * det));
    float lambda_min = (trace - sqrt_term) * 0.5f;
    float lambda_max = (trace + sqrt_term) * 0.5f;

    if (lambda_max < 1e-6f) return 0.0f;

    // Well-conditioned gradient matrix -> high confidence
    float texture_conf = lambda_min / (lambda_max + 1e-6f);
    // Absolute strength matters too
    float strength_conf = std::min(1.0f, lambda_min / 100.0f);
    float conf = texture_conf * strength_conf;

    // 2. Flow magnitude penalty: very large flows are suspicious
    float flow_mag = flow.length();
    if (flow_mag > config_.max_flow_magnitude * 0.5f) {
        conf *= std::exp(-(flow_mag - config_.max_flow_magnitude * 0.5f) / config_.max_flow_magnitude);
    }

    // 3. Clamp penalty: if flow was clamped, heavily distrust it
    if (was_clamped) {
        conf *= 0.1f;
    }

    return std::max(0.0f, std::min(1.0f, conf));
}

void OpticalFlowCalculator::detect_occlusions(OpticalFlowResult& result,
                                               const std::vector<float>& prev_frame,
                                               const std::vector<float>& curr_frame,
                                               int width, int height) const {
    // Compute backward flow (curr -> prev) for forward-backward consistency check
    OpticalFlowResult bw_result;
    bw_result.width = width;
    bw_result.height = height;
    bw_result.vectors.resize(result.vectors.size());
    compute_lk(curr_frame, prev_frame, width, height, bw_result, 0);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = static_cast<size_t>(y * width + x);
            if (!result.vectors[idx].valid) continue;

            math::Vector2 fw = result.vectors[idx].flow;
            int x2 = static_cast<int>(x + fw.x + 0.5f);
            int y2 = static_cast<int>(y + fw.y + 0.5f);

            // Out of bounds = occluded
            if (x2 < 0 || x2 >= width || y2 < 0 || y2 >= height) {
                result.vectors[idx].flow = math::Vector2::zero();
                result.vectors[idx].valid = false;
                result.vectors[idx].confidence *= 0.1f;
                continue;
            }

            // Forward-backward consistency: fw(p) + bw(p+fw) should be ~0
            size_t bw_idx = static_cast<size_t>(y2 * width + x2);
            math::Vector2 bw = bw_result.vectors[bw_idx].flow;
            math::Vector2 inconsistency = fw + bw;
            float error = inconsistency.length();

            if (error > config_.fb_consistency_threshold) {
                // Inconsistent = occluded or unreliable
                result.vectors[idx].flow = math::Vector2::zero();
                result.vectors[idx].valid = false;
                result.vectors[idx].confidence *= 0.3f;
            } else {
                // Small penalty proportional to inconsistency
                result.vectors[idx].confidence *= std::exp(-error * 0.5f);
            }
        }
    }
}

void OpticalFlowCalculator::temporal_smooth(OpticalFlowResult& result) {
    if (prev_result_.vectors.empty()
        || prev_result_.width != result.width
        || prev_result_.height != result.height) {
        return;
    }

    for (size_t i = 0; i < result.vectors.size() && i < prev_result_.vectors.size(); ++i) {
        auto& curr = result.vectors[i];
        const auto& prev = prev_result_.vectors[i];

        if (curr.valid && prev.valid) {
            // Both valid: smooth flow and confidence, increment age
            float alpha = 0.7f; // favour current
            curr.flow = curr.flow * alpha + prev.flow * (1.0f - alpha);
            curr.confidence = curr.confidence * alpha + prev.confidence * (1.0f - alpha);
            curr.age = static_cast<uint8_t>(std::min(255, static_cast<int>(prev.age) + 1));
        }
        else if (curr.valid && !prev.valid) {
            // Current valid, previous invalid: reset age (new track)
            curr.age = 0;
        }
        else if (!curr.valid && prev.valid) {
            // Current invalid, previous valid: do NOT resurrect here;
            // explicit_degrade has already zeroed the flow.
            // Leave age at 0.
            curr.age = 0;
        }
        // else both invalid: nothing to smooth
    }
}

void OpticalFlowCalculator::explicit_degrade(OpticalFlowResult& result) const {
    // Final pass: any vector that is still not trustworthy gets zeroed.
    // This guarantees the consumer never sees a "bad but non-zero" flow.
    for (auto& vec : result.vectors) {
        if (!vec.valid || vec.confidence < config_.confidence_threshold) {
            vec.flow = math::Vector2::zero();
            vec.valid = false;
            // Keep the (low) confidence value so the consumer can still
            // classify it as low / medium / high.
        }
    }
}

bool OpticalFlowCalculator::fallback_affine(OpticalFlowResult& result, int width, int height) const {
    // Estimate affine transform from valid vectors and apply to all
    std::vector<math::Vector2> src_pts, dst_pts;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = static_cast<size_t>(y * width + x);
            if (result.vectors[idx].valid && result.vectors[idx].confidence > 0.3f) {
                src_pts.push_back(math::Vector2(static_cast<float>(x), static_cast<float>(y)));
                dst_pts.push_back(math::Vector2(static_cast<float>(x), static_cast<float>(y)) + result.vectors[idx].flow);
            }
        }
    }

    if (src_pts.size() < 3) return false;

    // Simplified: just mark all invalid as zero-flow low-confidence
    for (auto& vec : result.vectors) {
        if (!vec.valid) {
            vec.flow = math::Vector2::zero();
            vec.confidence = 0.1f;
        }
    }
    return true;
}

// ============ Main Compute Methods ============

OpticalFlowResult OpticalFlowCalculator::compute(const std::vector<float>& prev_frame,
                                                  const std::vector<float>& curr_frame,
                                                  int width, int height, int /*channels*/) {
    OpticalFlowResult result;
    result.width = width;
    result.height = height;
    result.vectors.resize(static_cast<size_t>(width * height));

    compute_lk(prev_frame, curr_frame, width, height, result, 0);

    if (config_.enable_occlusion_detection) {
        detect_occlusions(result, prev_frame, curr_frame, width, height);
    }

    if (config_.enable_temporal_smoothing) {
        temporal_smooth(result);
    }

    const int valid_before_fallback = result.valid_count();
    if (valid_before_fallback < std::max(8, static_cast<int>(result.vectors.size() / 16))) {
        const TranslationEstimate estimate = estimate_translation(
            prev_frame,
            curr_frame,
            width,
            height,
            static_cast<int>(std::round(config_.max_flow_magnitude)));
        apply_translation_estimate(result, estimate);
    }

    explicit_degrade(result);

    return result;
}

OpticalFlowResult OpticalFlowCalculator::compute_multiscale(const std::vector<float>& prev_frame,
                                                             const std::vector<float>& curr_frame,
                                                             int width, int height, int /*channels*/) {
    OpticalFlowResult result = compute(prev_frame, curr_frame, width, height);
    const TranslationEstimate estimate = estimate_translation(
        prev_frame,
        curr_frame,
        width,
        height,
        static_cast<int>(std::round(config_.max_flow_magnitude)));
    apply_translation_estimate(result, estimate);
    return result;
}

} // namespace tachyon::tracker
