#pragma once
#include "tachyon/core/math/algebra/vector2.h"
#include <vector>
#include <cstdint>
#include <string>
#include <memory>

namespace tachyon::tracker {

// ---------------------------------------------------------------------------
// A. CONTRACT: Every vector always carries flow + confidence + valid.
//    The consumer never infers quality — the contract is explicit.
//    age / source_level are optional debug fields.
// ---------------------------------------------------------------------------
struct OpticalFlowVector {
    math::Vector2 flow;       // motion vector (dx, dy)
    float confidence{0.0f};   // confidence value [0, 1]
    bool valid{false};        // whether the vector is valid
    uint8_t age{0};           // how many frames this vector has been tracked
    int8_t source_level{-1};  // pyramid level where this was computed (-1 = unknown)

    OpticalFlowVector() = default;
    OpticalFlowVector(const math::Vector2& f, float c, bool v,
                      uint8_t a = 0, int8_t sl = -1)
        : flow(f), confidence(c), valid(v), age(a), source_level(sl) {}
};

struct OpticalFlowResult {
    std::vector<OpticalFlowVector> vectors;
    int width{0};
    int height{0};

    // Get flow at normalized position (x, y in [0, 1])
    const OpticalFlowVector* get_flow_at(float nx, float ny) const;

    // Get flow at pixel position
    const OpticalFlowVector* get_flow_at_pixel(int x, int y) const;

    // Get average confidence for the entire frame (valid vectors only)
    float average_confidence() const;

    // Count valid vectors
    int valid_count() const;
};

enum class FlowFallbackMode { kHold, kBlend, kAffine, kHomography, kPrevious };

// ---------------------------------------------------------------------------
// C. CONSUMER: explicit fallback decisions.
//    High   -> use flow
//    Medium -> blend
//    Low    -> hold or zero motion
// ---------------------------------------------------------------------------
class OpticalFlowConsumer {
public:
    struct Config {
        float high_confidence_threshold;   // Use flow directly
        float low_confidence_threshold;    // Fallback to blend/hold/zero
        float blend_factor;                // Blend ratio when confidence is medium
        bool enable_hold_for_low_conf;      // true = hold previous, false = zero motion

        Config() : high_confidence_threshold(0.7f),
                   low_confidence_threshold(0.3f),
                   blend_factor(0.5f),
                   enable_hold_for_low_conf(true) {}
    };

    enum class Action { kFlow, kBlend, kHold, kZeroMotion };

    struct ConsumerResult {
        Action action;
        math::Vector2 flow; // Raw flow from calculator (may be zero if degraded)
        float weight{1.0f}; // How much to apply (0 = none, 1 = full)
    };

    explicit OpticalFlowConsumer(const Config& config = Config());

    // Decide what to do for a single vector
    ConsumerResult consume(const OpticalFlowVector& vec) const;

    // Process entire flow field and produce output flow with fallback applied.
    // prev_flow is used for kHold fallback.
    OpticalFlowResult process(const OpticalFlowResult& flow,
                              const OpticalFlowResult& prev_flow = OpticalFlowResult{}) const;

    Config& config() { return config_; }
    const Config& config() const { return config_; }

private:
    Config config_;
};

// ---------------------------------------------------------------------------
// B. CALCULATOR: produces real confidence from:
//    - gradient response
//    - forward/backward consistency
//    - occlusion detection
//    - clamp for large flows
//    - temporal smoothing
// ---------------------------------------------------------------------------
class OpticalFlowCalculator {
public:
    struct Config {
        int pyramid_levels;
        int window_size;
        int iterations;
        float confidence_threshold;      // Below this -> explicitly degraded
        bool enable_temporal_smoothing;
        bool enable_occlusion_detection;
        FlowFallbackMode fallback;
        float eigenvalue_threshold;     // Minimum eigenvalue for valid flow
        float max_flow_magnitude;       // Flow above this is heavily penalised
        float fb_consistency_threshold;  // Max forward-backward error in pixels

        Config() : pyramid_levels(3),
                   window_size(15),
                   iterations(3),
                   confidence_threshold(0.3f),
                   enable_temporal_smoothing(true),
                   enable_occlusion_detection(true),
                   fallback(FlowFallbackMode::kPrevious),
                   eigenvalue_threshold(0.01f),
                   max_flow_magnitude(50.0f),
                   fb_consistency_threshold(2.0f) {}
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

    // Calculate confidence based on eigenvalues, flow magnitude, and optional penalties
    float calculate_confidence(float Ixx, float Ixy, float Iyy,
                               const math::Vector2& flow,
                               bool was_clamped) const;

    // Detect occlusions using forward-backward consistency.
    // Computes backward flow (curr->prev) and penalises inconsistent vectors.
    void detect_occlusions(OpticalFlowResult& result,
                           const std::vector<float>& prev_frame,
                           const std::vector<float>& curr_frame,
                           int width, int height) const;

    // Apply temporal smoothing to flow vectors and propagate age
    void temporal_smooth(OpticalFlowResult& result);

    // Explicitly degrade vectors whose confidence is below threshold.
    // Flow is zeroed so the consumer never has to "infer" badness.
    void explicit_degrade(OpticalFlowResult& result) const;

    // Fallback to affine transformation
    bool fallback_affine(OpticalFlowResult& result, int width, int height) const;

    Config config_;
    OpticalFlowResult prev_result_;
};

} // namespace tachyon::tracker

