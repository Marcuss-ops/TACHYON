#include "tachyon/tracker/optical_flow.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace tachyon::tracker {

struct Matrix2x2 {
    float m[2][2];
    
    float det() const {
        return m[0][0] * m[1][1] - m[0][1] * m[1][0];
    }
    
    Matrix2x2 inverse() const {
        float d = det();
        Matrix2x2 inv;
        if (std::abs(d) > 1e-6f) {
            inv.m[0][0] = m[1][1] / d;
            inv.m[0][1] = -m[0][1] / d;
            inv.m[1][0] = -m[1][0] / d;
            inv.m[1][1] = m[0][0] / d;
        }
        return inv;
    }
};

struct Vector2f {
    float x, y;
    Vector2f operator*(const Matrix2x2& m) const {
        return {x * m.m[0][0] + y * m.m[1][0], x * m.m[0][1] + y * m.m[1][1]};
    }
};

OpticalFlowCalculator::OpticalFlowCalculator(const Config& config) : config_(config) {}
OpticalFlowCalculator::~OpticalFlowCalculator() = default;

const OpticalFlowVector* OpticalFlowResult::get_flow_at(float nx, float ny) const {
    int x = static_cast<int>(nx * width);
    int y = static_cast<int>(ny * height);
    x = std::max(0, std::min(x, width - 1));
    y = std::max(0, std::min(y, height - 1));
    size_t idx = static_cast<size_t>(y * width + x);
    if (idx < vectors.size()) return &vectors[idx];
    return nullptr;
}

// Simplified implementations - full LK would be more complex
OpticalFlowResult OpticalFlowCalculator::compute(const std::vector<float>& prev_frame,
                                                const std::vector<float>& curr_frame,
                                                int width, int height, int channels) {
    OpticalFlowResult result;
    result.width = width;
    result.height = height;
    result.vectors.resize(static_cast<size_t>(width * height));
    
    // Simplified: just mark as valid with zero flow for now
    for (auto& v : result.vectors) {
        v.flow = math::Vector2{0.0f, 0.0f};
        v.confidence = 0.5f;
        v.valid = true;
    }
    
    return result;
}

OpticalFlowResult OpticalFlowCalculator::compute_multiscale(const std::vector<float>& prev_frame,
                                                           const std::vector<float>& curr_frame,
                                                           int width, int height, int channels) {
    return compute(prev_frame, curr_frame, width, height, channels);
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
}

void OpticalFlowCalculator::compute_lk(const std::vector<float>& prev, const std::vector<float>& curr,
                                       int width, int height, OpticalFlowResult& result, int level) const {
    // Simplified - would implement full LK here
}

float OpticalFlowCalculator::calculate_confidence(const std::vector<float>& window1, 
                                               const std::vector<float>& window2,
                                               const math::Vector2& flow) const {
    if (window1.empty() || window2.empty()) return 0.0f;
    float var1 = 0.0f, var2 = 0.0f;
    float mean1 = std::accumulate(window1.begin(), window1.end(), 0.0f) / window1.size();
    float mean2 = std::accumulate(window2.begin(), window2.end(), 0.0f) / window2.size();
    
    for (float v : window1) var1 += (v - mean1) * (v - mean1);
    for (float v : window2) var2 += (v - mean2) * (v - mean2);
    
    float snr = (var1 + var2) > 0.0f ? 1.0f / (var1 + var2) : 0.0f;
    return std::max(0.0f, std::min(1.0f, snr));
}

void OpticalFlowCalculator::detect_occlusions(OpticalFlowResult& result, 
                                             const std::vector<float>& prev_frame,
                                             const std::vector<float>& curr_frame,
                                             int width, int height) const {
    // Simplified - would implement forward-backward consistency
}

void OpticalFlowCalculator::temporal_smooth(OpticalFlowResult& result) const {
    if (prev_result_.vectors.empty()) return;
    for (size_t i = 0; i < result.vectors.size() && i < prev_result_.vectors.size(); ++i) {
        if (result.vectors[i].valid && prev_result_.vectors[i].valid) {
            result.vectors[i].flow = result.vectors[i].flow * 0.7f + prev_result_.vectors[i].flow * 0.3f;
        }
    }
}

bool OpticalFlowCalculator::fallback_affine(OpticalFlowResult& result, int width, int height) const {
    for (auto& vec : result.vectors) {
        vec.valid = false;
        vec.confidence = 0.0f;
    }
    return false;
}

} // namespace tachyon::tracker
