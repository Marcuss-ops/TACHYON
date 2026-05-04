#pragma once

#include "tachyon/core/api.h"
#include "tachyon/timeline/time_remap.h"
#include <vector>
#include <cstdint>

namespace tachyon::timeline {

struct FrameBuffer {
    std::vector<uint8_t> data;
    int width{0};
    int height{0};
    int channels{4}; // RGBA
};

struct OpticalFlowField {
    std::vector<float> flow_x; // horizontal displacement
    std::vector<float> flow_y; // vertical displacement
    int width{0};
    int height{0};

    std::pair<float, float> at(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) return {0.0f, 0.0f};
        const int idx = (y * width + x);
        return {flow_x[idx], flow_y[idx]};
    }
};

/**
 * @brief Abstract interface for optical flow and frame synthesis algorithms
 */
class TACHYON_API IOpticalFlowProvider {
public:
    virtual ~IOpticalFlowProvider() = default;

    /**
     * @brief Compute optical flow between two frames
     */
    virtual OpticalFlowField compute_optical_flow(const FrameBuffer& frame_a, const FrameBuffer& frame_b) const = 0;

    /**
     * @brief Warp a frame using optical flow vectors
     */
    virtual FrameBuffer warp_optical_flow(const FrameBuffer& frame, const OpticalFlowField& flow, float t) const = 0;

    /**
     * @brief Synthesize an intermediate frame using optical flow
     */
    virtual FrameBuffer synthesize_frame_optical_flow(
        const FrameBuffer& frame_a,
        const FrameBuffer& frame_b,
        const OpticalFlowField& flow_ab,
        const OpticalFlowField& flow_ba,
        float t) const = 0;
};

/**
 * @brief Default optical flow provider using sparse tracking and IDW interpolation
 */
class TACHYON_API DefaultOpticalFlowProvider : public IOpticalFlowProvider {
public:
    OpticalFlowField compute_optical_flow(const FrameBuffer& frame_a, const FrameBuffer& frame_b) const override;
    FrameBuffer warp_optical_flow(const FrameBuffer& frame, const OpticalFlowField& flow, float t) const override;
    FrameBuffer synthesize_frame_optical_flow(
        const FrameBuffer& frame_a,
        const FrameBuffer& frame_b,
        const OpticalFlowField& flow_ab,
        const OpticalFlowField& flow_ba,
        float t) const override;
};

/**
 * @brief Blend two frames linearly based on blend factor
 */
TACHYON_API FrameBuffer blend_linear(const FrameBuffer& a, const FrameBuffer& b, float factor);

} // namespace tachyon::timeline
