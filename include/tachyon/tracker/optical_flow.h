#pragma once
#include "tachyon/core/math/vector2.h"
#include <vector>
#include <cstdint>
#include <string>

namespace tachyon::tracker {

struct OpticalFlowVector {
    math::Vector2 flow; // motion vector (dx, dy)
    float confidence{0.0f}; // confidence value [0, 1]
    bool valid{false}; // whether the vector is valid
};

struct OpticalFlowResult {
    std::vector<OpticalFlowVector> vectors;
    int width{0};
    int height{0};
    
    // Get flow at normalized position (x, y in [0, 1])
    const OpticalFlowVector* get_flow_at(float nx, float ny) const;
};

enum class FlowFallbackMode { kHold, kAffine, kHomography, kPrevious };

class OpticalFlowCalculator {
public:
    struct Config {
        int pyramid_levels;
        int window_size;
        int iterations;
        float confidence_threshold;
        bool enable_temporal_smoothing;
        FlowFallbackMode fallback;
        
        Config() 
            : pyramid_levels(3)
            , window_size(15)
            , iterations(3)
            , confidence_threshold(0.3f)
            , enable_temporal_smoothing(true)
            , fallback(FlowFallbackMode::kPrevious) {}
    };

    explicit OpticalFlowCalculator(const Config& config = Config());
    ~OpticalFlowCalculator() = default;

    // Compute optical flow between two frames
    OpticalFlowResult compute(const std::vector<float>& prev_frame,
                             const std::vector<float>& curr_frame,
                             int width, int height, int channels = 1);

    // Compute multi-scale flow with pyramid
    OpticalFlowResult compute_multiscale(const std::vector<float>& prev_frame,
                                        const std::vector<float>& curr_frame,
                                        int width, int height, int channels = 1);

    // Set previous result for temporal smoothing
    void set_previous_result(const OpticalFlowResult& prev) { prev_result_ = prev; }

    Config& config() { return config_; }
    const Config& config() const { return config_; }

private:
    // Build Gaussian pyramid
    void build_pyramid(const std::vector<float>& frame, int width, int height,
                       std::vector<std::vector<float>>& pyramid, std::vector<int>& widths,
                       std::vector<int>& heights) const;

    // Lucas-Kanade flow at single scale
    void compute_lk(const std::vector<float>& prev, const std::vector<float>& curr,
                    int width, int height, OpticalFlowResult& result, int level = 0) const;

    // Calculate confidence based on signal-to-noise ratio
    float calculate_confidence(const std::vector<float>& window1, const std::vector<float>& window2,
                              const math::Vector2& flow) const;

    // Detect occlusions using forward-backward consistency
    void detect_occlusions(OpticalFlowResult& result, const std::vector<float>& prev_frame,
                          const std::vector<float>& curr_frame, int width, int height) const;

    // Apply temporal smoothing to flow vectors
    void temporal_smooth(OpticalFlowResult& result) const;

    // Fallback to affine transformation
    bool fallback_affine(OpticalFlowResult& result, int width, int height) const;

    Config config_;
    OpticalFlowResult prev_result_;
};

} // namespace tachyon::tracker
