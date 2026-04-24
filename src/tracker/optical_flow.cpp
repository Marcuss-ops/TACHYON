#include "tachyon/tracker/optical_flow.h"
#include <cmath>
#include <algorithm>
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
    
    if (!vec.valid || vec.confidence < config_.low_confidence_threshold) {
        result.action = config_.enable_hold_for_low_conf ? Action::kHold : Action::kBlend;
        result.flow = math::Vector2::zero();
        result.weight = 0.0f;
    } else if (vec.confidence < config_.high_confidence_threshold) {
        result.action = Action::kBlend;
        result.flow = vec.flow;
        result.weight = config_.blend_factor * vec.confidence;
    } else {
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
    
    for (size_t i = 0; i < flow.vectors.size(); ++i) {
        auto consumer_result = consume(flow.vectors[i]);
        
        if (consumer_result.action == Action::kHold && !prev_flow.vectors.empty() && i < prev_flow.vectors.size()) {
            result.vectors[i] = prev_flow.vectors[i];
        } else {
            math::Vector2 final_flow = consumer_result.flow * consumer_result.weight;
            float conf = consumer_result.weight;
            result.vectors[i] = OpticalFlowVector(final_flow, conf, consumer_result.weight > 0.01f);
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
            
            size_t idx = static_cast<size_t>(y * width + x);
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
                                         int width, int height, OpticalFlowResult& result, int /*level*/) const {
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
                continue;
            }
            
            Matrix2x2 M{{Ixx, Ixy, Ixy, Iyy}};
            float det = M.det();
            
            math::Vector2 flow;
            if (std::abs(det) > config_.eigenvalue_threshold) {
                Matrix2x2 Minv = M.inverse();
                math::Vector2 neg_b{-Ixt, -Iyt};
                flow = math::Vector2(
                    Minv.m[0][0] * neg_b.x + Minv.m[0][1] * neg_b.y,
                    Minv.m[1][0] * neg_b.x + Minv.m[1][1] * neg_b.y
                );
                
                // Clamp flow to reasonable range
                float max_flow = static_cast<float>(std::max(width, height)) * 0.5f;
                flow.x = std::max(-max_flow, std::min(max_flow, flow.x));
                flow.y = std::max(-max_flow, std::min(max_flow, flow.y));
                
                result.vectors[idx].flow = flow;
                result.vectors[idx].confidence = calculate_confidence(Ixx, Ixy, Iyy, flow);
                result.vectors[idx].valid = result.vectors[idx].confidence > config_.confidence_threshold;
            } else {
                result.vectors[idx].valid = false;
                result.vectors[idx].confidence = 0.0f;
            }
        }
    }
}

float OpticalFlowCalculator::calculate_confidence(float Ixx, float Ixy, float Iyy, const math::Vector2& flow) const {
    // Use eigenvalue ratio/min eigenvalue as confidence
    // High texture areas have large eigenvalues
    float trace = Ixx + Iyy;
    float det = Ixx * Iyy - Ixy * Ixy;
    
    if (trace < 1e-6f) return 0.0f;
    
    // Corner response (Harris-like)
    float response = (det > 0) ? (det / (trace + 1e-6f)) : 0.0f;
    
    // Normalize to [0, 1] - typical values are small
    float confidence = std::min(1.0f, response * 1000.0f);
    
    // Also factor in flow magnitude - very large flows may be incorrect
    float flow_mag = flow.length();
    if (flow_mag > 50.0f) {
        confidence *= std::exp(-(flow_mag - 50.0f) / 100.0f);
    }
    
    return std::max(0.0f, std::min(1.0f, confidence));
}

void OpticalFlowCalculator::detect_occlusions(OpticalFlowResult& result, 
                                               const std::vector<float>& /*prev_frame*/,
                                               const std::vector<float>& /*curr_frame*/,
                                               int width, int height) const {
    // Forward-backward consistency check
    // For each vector, check if warping back gives consistent result
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = static_cast<size_t>(y * width + x);
            if (!result.vectors[idx].valid) continue;
            
            math::Vector2 flow = result.vectors[idx].flow;
            int x2 = static_cast<int>(x + flow.x + 0.5f);
            int y2 = static_cast<int>(y + flow.y + 0.5f);
            
            if (x2 < 0 || x2 >= width || y2 < 0 || y2 >= height) {
                result.vectors[idx].valid = false;
                result.vectors[idx].confidence *= 0.5f;
                continue;
            }
            
            // Check backward consistency (simplified - would need backward flow)
            // For now, just check if flow magnitude is reasonable
            if (flow.length() > std::max(width, height) * 0.25f) {
                result.vectors[idx].valid = false;
                result.vectors[idx].confidence *= 0.3f;
            }
        }
    }
}

void OpticalFlowCalculator::temporal_smooth(OpticalFlowResult& result) const {
    if (prev_result_.vectors.empty() || prev_result_.width != result.width || prev_result_.height != result.height) {
        return;
    }
    
    for (size_t i = 0; i < result.vectors.size() && i < prev_result_.vectors.size(); ++i) {
        if (result.vectors[i].valid && prev_result_.vectors[i].valid) {
            // Blend current and previous
            float alpha = 0.7f;
            result.vectors[i].flow = result.vectors[i].flow * alpha + prev_result_.vectors[i].flow * (1.0f - alpha);
            result.vectors[i].confidence = (result.vectors[i].confidence + prev_result_.vectors[i].confidence) * 0.5f;
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
    
    // Simplified: just mark all as invalid to trigger hold
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
    
    return result;
}

OpticalFlowResult OpticalFlowCalculator::compute_multiscale(const std::vector<float>& prev_frame,
                                                            const std::vector<float>& curr_frame,
                                                            int width, int height, int /*channels*/) {
    std::vector<std::vector<float>> pyr_prev, pyr_curr;
    std::vector<int> widths, heights;
    
    build_pyramid(prev_frame, width, height, pyr_prev, widths, heights);
    build_pyramid(curr_frame, width, height, pyr_curr, widths, heights);
    
    OpticalFlowResult result;
    result.width = width;
    result.height = height;
    result.vectors.resize(static_cast<size_t>(width * height));
    
    // Initialize with zero flow
    for (auto& v : result.vectors) {
        v.flow = math::Vector2::zero();
        v.confidence = 1.0f;
        v.valid = true;
    }
    
    // Process from coarse to fine
    for (int level = static_cast<int>(pyr_prev.size()) - 1; level >= 0; --level) {
        int w = widths[level];
        int h = heights[level];
        
        // Upscale flow from coarser level
        if (level < static_cast<int>(pyr_prev.size()) - 1) {
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    // Map to finer level coordinates
                    float fx = static_cast<float>(x) * 2.0f;
                    float fy = static_cast<float>(y) * 2.0f;
                    int src_x = static_cast<int>(fx);
                    int src_y = static_cast<int>(fy);
                    
                    if (src_x >= 0 && src_x < w && src_y >= 0 && src_y < h) {
                        size_t dst_idx = static_cast<size_t>(y * w + x);
                        size_t src_idx = static_cast<size_t>(src_y * (w * 2) + src_x * 2);
                        if (dst_idx < result.vectors.size() && src_idx < result.vectors.size()) {
                            result.vectors[dst_idx].flow = result.vectors[src_idx].flow * 2.0f;
                        }
                    }
                }
            }
        }
        
        // Compute LK at this level
        compute_lk(pyr_prev[level], pyr_curr[level], w, h, result, level);
    }
    
    if (config_.enable_occlusion_detection) {
        detect_occlusions(result, prev_frame, curr_frame, width, height);
    }
    
    if (config_.enable_temporal_smoothing) {
        temporal_smooth(result);
    }
    
    return result;
}

} // namespace tachyon::tracker
