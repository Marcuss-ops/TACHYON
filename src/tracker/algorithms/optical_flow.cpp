#include "tachyon/tracker/algorithms/optical_flow.h"
#include "tachyon/tracker/algorithms/optical_flow_math.h"
#include <cmath>
#include <algorithm>

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

TranslationEstimate estimate_translation(
    const std::vector<float>& prev,
    const std::vector<float>& curr,
    int width,
    int height,
    int max_shift);

void apply_translation_estimate(
    OpticalFlowResult& result,
    const TranslationEstimate& estimate);

OpticalFlowCalculator::OpticalFlowCalculator(const Config& config) : config_(config) {}

void OpticalFlowCalculator::build_pyramid(const std::vector<float>& frame, int width, int height,
                                           std::vector<std::vector<float>>& pyramid,
                                           std::vector<int>& widths,
                                           std::vector<int>& heights) const {
    OpticalFlowMath::build_pyramid(frame, width, height, config_.pyramid_levels, pyramid, widths, heights);
}

bool OpticalFlowCalculator::compute_gradient_matrices(const std::vector<float>& prev, const std::vector<float>& curr,
                                                       int width, int height, int cx, int cy, int window_half,
                                                       float& Ixx, float& Ixy, float& Iyy, float& Ixt, float& Iyt) const {
    OpticalFlowMath::Gradients g;
    bool result = OpticalFlowMath::compute_gradients(prev, curr, width, height, cx, cy, window_half, g);
    Ixx = g.Ixx; Ixy = g.Ixy; Iyy = g.Iyy; Ixt = g.Ixt; Iyt = g.Iyt;
    return result;
}

void OpticalFlowCalculator::compute_lk(const std::vector<float>& prev, const std::vector<float>& curr,
                                        int width, int height, OpticalFlowResult& result, int level) const {
    int window_half = config_.window_size / 2;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = static_cast<size_t>(y * width + x);
            if (idx >= result.vectors.size()) continue;

            OpticalFlowMath::Gradients g;
            if (!OpticalFlowMath::compute_gradients(prev, curr, width, height, x, y, window_half, g)) {
                result.vectors[idx].valid = false;
                result.vectors[idx].confidence = 0.0f;
                result.vectors[idx].flow = math::Vector2::zero();
                result.vectors[idx].source_level = static_cast<int8_t>(level);
                continue;
            }

            bool was_clamped = false;
            math::Vector2 flow = OpticalFlowMath::solve_lk(g, config_.eigenvalue_threshold, config_.max_flow_magnitude, was_clamped);

            if (flow.x != 0.0f || flow.y != 0.0f || was_clamped) {
                float conf = calculate_confidence(g.Ixx, g.Ixy, g.Iyy, flow, was_clamped);
                bool valid = conf > config_.confidence_threshold;

                result.vectors[idx].flow = flow;
                result.vectors[idx].confidence = conf;
                result.vectors[idx].valid = valid;
                result.vectors[idx].source_level = static_cast<int8_t>(level);

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
    return OpticalFlowMath::calculate_confidence(Ixx, Ixy, Iyy, flow, was_clamped, config_.max_flow_magnitude);
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
            float error = OpticalFlowMath::compute_forward_backward_error(fw, bw);

            if (error > config_.fb_consistency_threshold) {
                // Inconsistent = occluded or unreliable
                result.vectors[idx].flow = math::Vector2::zero();
                result.vectors[idx].valid = false;
                result.vectors[idx].confidence *= 0.3f;
            } else {
                // Small penalty proportional to inconsistency
                result.vectors[idx].confidence = OpticalFlowMath::apply_occlusion_confidence_penalty(result.vectors[idx].confidence, error);
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
            constexpr float alpha = 0.7f; // favour current
            curr.flow = OpticalFlowMath::smooth_flow(curr.flow, prev.flow, alpha);
            curr.confidence = OpticalFlowMath::smooth_confidence(curr.confidence, prev.confidence, alpha);
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

