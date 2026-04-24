#pragma once
#include "tachyon/core/math/vector2.h"
#include <vector>
#include <cstdint>
#include <string>
#include <memory>

namespace tachyon::tracker {

struct OpticalFlowVector {
    math::Vector2 flow; // motion vector (dx, dy)
    float confidence{0.0f}; // confidence value [0, 1]
    bool valid{false}; // whether the vector is valid
    
    OpticalFlowVector() = default;
    OpticalFlowVector(const math::Vector2& f, float c, bool v) 
        : flow(f), confidence(c), valid(v) {}
};

struct OpticalFlowResult {
    std::vector<OpticalFlowVector> vectors;
    int width{0};
    int height{0};
    
    // Get flow at normalized position (x, y in [0, 1])
    const OpticalFlowVector* get_flow_at(float nx, float ny) const;
    
    // Get flow at pixel position
    const OpticalFlowVector* get_flow_at_pixel(int x, int y) const;
    
    // Get average confidence for the entire frame
    float average_confidence() const;
    
    // Count valid vectors
    int valid_count() const;
};

enum class FlowFallbackMode { kHold, kBlend, kAffine, kHomography, kPrevious };

// Consumer that decides flow/blend/hold based on confidence
class OpticalFlowConsumer {
public:
    struct Config {
        float high_confidence_threshold{0.7f};  // Use flow
        float low_confidence_threshold{0.3f};   // Fallback to blend/hold
        float blend_factor{0.5f};               // Blend ratio when confidence is medium
        bool enable_hold_for_low_conf{true};     // Hold previous frame when confidence very low
        
        Config() = default;
    };
    
    enum class Action { kFlow, kBlend, kHold };
    
    struct ConsumerResult {
        Action action;
        math::Vector2 flow; // Final flow to apply (may be blended or zero)
        float weight{1.0f}; // How much to apply (0=none, 1=full)
    };
    
    OpticalFlowConsumer(const Config& config = Config());
    
    // Decide what to do for a single vector
    ConsumerResult consume(const OpticalFlowVector& vec) const;
    
    // Process entire flow field and produce output flow with fallback applied
    OpticalFlowResult process(const OpticalFlowResult& flow, 
                             const OpticalFlowResult& prev_flow = OpticalFlowResult{}) const;
    
    Config& config() { return config_; }
    const Config& config() const { return config_; }
    
private:
    Config config_;
};

class OpticalFlowCalculator {
public:
    struct Config {
        int pyramid_levels{3};
        int window_size{15};
        int iterations{3};
        float confidence_threshold{0.3f};
        bool enable_temporal_smoothing{true};
        bool enable_occlusion_detection{true};
        FlowFallbackMode fallback{FlowFallbackMode::kPrevious};
        float eigenvalue_threshold{0.01f}; // Minimum eigenvalue for valid flow
        
        Config() = default;
    };

    explicit OpticalFlowCalculator(const Config& config = Config());
    ~OpticalFlowCalculator() = default;

    // Compute optical flow between two frames
    OpticalFlowResult compute(const std::vector<float>& prev_frame,
                             const std::vector<float>& curr_frame,
                             int width, int height, int channels = 1);

    // Compute multi-scale flow with pyramid (proper implementation)
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

    // Downsample image by factor of 2
    void downsample(const std::vector<float>& src, int src_w, int src_h,
                    std::vector<float>& dst, int& dst_w, int& dst_h) const;

    // Lucas-Kanade flow at single scale
    void compute_lk(const std::vector<float>& prev, const std::vector<float>& curr,
                    int width, int height, OpticalFlowResult& result, int level = 0) const;

    // Compute spatial gradient matrices (Ix, Iy, It) for a window
    bool compute_gradient_matrices(const std::vector<float>& prev, const std::vector<float>& curr,
                                   int width, int height, int cx, int cy, int window_half,
                                   float& Ixx, float& Ixy, float& Iyy, float& Ixt, float& Iyt) const;

    // Calculate confidence based on eigenvalues of gradient matrix
    float calculate_confidence(float Ixx, float Ixy, float Iyy, const math::Vector2& flow) const;

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
