// frame_blend.cpp — Optical Flow bridge using OpticalFlowCalculator (LK pyramidal)
#include "tachyon/timeline/frame_blend.h"
#include "tachyon/tracker/optical_flow.h"
#include <algorithm>
#include <cmath>

namespace tachyon::timeline {

namespace {

inline uint8_t lerp_u8(uint8_t a, uint8_t b, float t) {
    return static_cast<uint8_t>(a + (b - a) * t);
}

inline float clamp01(float v) {
    return std::clamp(v, 0.0f, 1.0f);
}

float sample_bilinear(const std::vector<uint8_t>& img, int w, int h, int c, float x, float y, int channel) {
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int x1 = x0 + 1;
    const int y1 = y0 + 1;
    const float fx = x - x0;
    const float fy = y - y0;
    const auto get = [&](int px, int py) -> float {
        if (px < 0 || px >= w || py < 0 || py >= h) return 0.0f;
        return static_cast<float>(img[(py * w + px) * c + channel]) / 255.0f;
    };
    const float v00 = get(x0, y0);
    const float v10 = get(x1, y0);
    const float v01 = get(x0, y1);
    const float v11 = get(x1, y1);
    const float v0 = v00 * (1.0f - fx) + v10 * fx;
    const float v1 = v01 * (1.0f - fx) + v11 * fx;
    return v0 * (1.0f - fy) + v1 * fy;
}

// Convert RGBA FrameBuffer to grayscale float for OpticalFlowCalculator
std::vector<float> to_gray_float(const FrameBuffer& fb) {
    std::vector<float> gray(static_cast<size_t>(fb.width) * fb.height);
    for (int y = 0; y < fb.height; ++y) {
        for (int x = 0; x < fb.width; ++x) {
            const size_t idx = static_cast<size_t>(y * fb.width + x) * fb.channels;
            float r = fb.data[idx] / 255.0f;
            float g = (fb.channels > 1) ? fb.data[idx + 1] / 255.0f : r;
            float b = (fb.channels > 2) ? fb.data[idx + 2] / 255.0f : r;
            gray[static_cast<size_t>(y * fb.width + x)] = 0.299f * r + 0.587f * g + 0.114f * b;
        }
    }
    return gray;
}

// Convert dense OpticalFlowResult → OpticalFlowField used by the warp pipeline
OpticalFlowField to_flow_field(const tracker::OpticalFlowResult& res) {
    OpticalFlowField field;
    field.width  = res.width;
    field.height = res.height;
    field.flow_x.assign(res.vectors.size(), 0.0f);
    field.flow_y.assign(res.vectors.size(), 0.0f);
    for (size_t i = 0; i < res.vectors.size(); ++i) {
        if (res.vectors[i].valid) {
            field.flow_x[i] = res.vectors[i].flow.x;
            field.flow_y[i] = res.vectors[i].flow.y;
        }
    }
    return field;
}

} // namespace

// ---------------------------------------------------------------------------
// Free function: simple linear blend
// ---------------------------------------------------------------------------
FrameBuffer blend_linear(const FrameBuffer& a, const FrameBuffer& b, float factor) {
    factor = clamp01(factor);
    if (a.data.empty()) return b;
    if (b.data.empty()) return a;

    FrameBuffer result;
    result.width    = std::max(a.width,    b.width);
    result.height   = std::max(a.height,   b.height);
    result.channels = std::max(a.channels, b.channels);
    result.data.resize(static_cast<size_t>(result.width) * result.height * result.channels, 0);

    for (int y = 0; y < result.height; ++y) {
        for (int x = 0; x < result.width; ++x) {
            for (int c = 0; c < result.channels; ++c) {
                uint8_t va = 0, vb = 0;
                if (y < a.height && x < a.width && c < a.channels)
                    va = a.data[(y * a.width  + x) * a.channels + c];
                if (y < b.height && x < b.width && c < b.channels)
                    vb = b.data[(y * b.width  + x) * b.channels + c];
                result.data[(y * result.width + x) * result.channels + c] = lerp_u8(va, vb, factor);
            }
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// DefaultOpticalFlowProvider
// ---------------------------------------------------------------------------

FrameBuffer DefaultOpticalFlowProvider::warp_optical_flow(
    const FrameBuffer& frame, const OpticalFlowField& flow, float t) const {

    if (frame.data.empty() || flow.flow_x.empty()) return frame;

    FrameBuffer result;
    result.width    = frame.width;
    result.height   = frame.height;
    result.channels = frame.channels;
    result.data.resize(frame.data.size(), 0);

    for (int y = 0; y < frame.height; ++y) {
        for (int x = 0; x < frame.width; ++x) {
            const auto [fx, fy] = flow.at(x, y);
            const float src_x = static_cast<float>(x) + fx * t;
            const float src_y = static_cast<float>(y) + fy * t;
            for (int c = 0; c < frame.channels; ++c) {
                const float s = sample_bilinear(
                    frame.data, frame.width, frame.height, frame.channels, src_x, src_y, c);
                result.data[(y * frame.width + x) * frame.channels + c] =
                    static_cast<uint8_t>(std::clamp(s * 255.0f, 0.0f, 255.0f));
            }
        }
    }
    return result;
}

FrameBuffer DefaultOpticalFlowProvider::synthesize_frame_optical_flow(
    const FrameBuffer& frame_a,
    const FrameBuffer& frame_b,
    const OpticalFlowField& flow_ab,
    const OpticalFlowField& flow_ba,
    float t) const {

    if (t <= 0.0f) return frame_a;
    if (t >= 1.0f) return frame_b;

    const FrameBuffer warped_a = warp_optical_flow(frame_a, flow_ab,         t);
    const FrameBuffer warped_b = warp_optical_flow(frame_b, flow_ba, 1.0f - t);
    return blend_linear(warped_a, warped_b, t);
}

OpticalFlowField DefaultOpticalFlowProvider::compute_optical_flow(
    const FrameBuffer& frame_a,
    const FrameBuffer& frame_b) const {

    const int w = std::min(frame_a.width,  frame_b.width);
    const int h = std::min(frame_a.height, frame_b.height);
    if (w <= 0 || h <= 0) return {};

    const std::vector<float> gray_a = to_gray_float(frame_a);
    const std::vector<float> gray_b = to_gray_float(frame_b);

    // Dense LK pyramidal optical flow (forward-backward + occlusion check)
    tracker::OpticalFlowCalculator calculator;
    const tracker::OpticalFlowResult res = calculator.compute_multiscale(gray_a, gray_b, w, h);

    return to_flow_field(res);
}

} // namespace tachyon::timeline
