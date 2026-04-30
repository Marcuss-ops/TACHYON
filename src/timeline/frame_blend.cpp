#include "tachyon/timeline/frame_blend.h"
#include "tachyon/tracker/feature_tracker.h"
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

} // namespace

FrameBuffer blend_linear(const FrameBuffer& a, const FrameBuffer& b, float factor) {
    factor = clamp01(factor);
    if (a.data.empty()) return b;
    if (b.data.empty()) return a;

    FrameBuffer result;
    result.width = std::max(a.width, b.width);
    result.height = std::max(a.height, b.height);
    result.channels = std::max(a.channels, b.channels);
    result.data.resize(static_cast<size_t>(result.width) * result.height * result.channels, 0);

    for (int y = 0; y < result.height; ++y) {
        for (int x = 0; x < result.width; ++x) {
            for (int c = 0; c < result.channels; ++c) {
                uint8_t va = 0, vb = 0;
                if (y < a.height && x < a.width && c < a.channels) {
                    va = a.data[(y * a.width + x) * a.channels + c];
                }
                if (y < b.height && x < b.width && c < b.channels) {
                    vb = b.data[(y * b.width + x) * b.channels + c];
                }
                result.data[(y * result.width + x) * result.channels + c] = lerp_u8(va, vb, factor);
            }
        }
    }
    return result;
}

FrameBuffer DefaultOpticalFlowProvider::warp_optical_flow(const FrameBuffer& frame, const OpticalFlowField& flow, float t) const {
    if (frame.data.empty() || flow.flow_x.empty()) return frame;

    FrameBuffer result;
    result.width = frame.width;
    result.height = frame.height;
    result.channels = frame.channels;
    result.data.resize(frame.data.size(), 0);

    for (int y = 0; y < frame.height; ++y) {
        for (int x = 0; x < frame.width; ++x) {
            const auto [fx, fy] = flow.at(x, y);
            const float src_x = static_cast<float>(x) + fx * t;
            const float src_y = static_cast<float>(y) + fy * t;

            for (int c = 0; c < frame.channels; ++c) {
                const float sampled = sample_bilinear(frame.data, frame.width, frame.height, frame.channels, src_x, src_y, c);
                result.data[(y * frame.width + x) * frame.channels + c] = static_cast<uint8_t>(std::clamp(sampled * 255.0f, 0.0f, 255.0f));
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

    // Forward warping from A
    const FrameBuffer warped_a = warp_optical_flow(frame_a, flow_ab, t);

    // Backward warping from B
    const FrameBuffer warped_b = warp_optical_flow(frame_b, flow_ba, 1.0f - t);

    // Blend based on t
    return blend_linear(warped_a, warped_b, t);
}

OpticalFlowField DefaultOpticalFlowProvider::compute_optical_flow(const FrameBuffer& frame_a, const FrameBuffer& frame_b) const {
    OpticalFlowField result;
    result.width = std::min(frame_a.width, frame_b.width);
    result.height = std::min(frame_a.height, frame_b.height);
    result.flow_x.assign(static_cast<size_t>(result.width) * result.height, 0.0f);
    result.flow_y.assign(static_cast<size_t>(result.width) * result.height, 0.0f);

    std::vector<float> pixels_a(static_cast<size_t>(result.width) * result.height);
    std::vector<float> pixels_b(static_cast<size_t>(result.width) * result.height);

    // Convert RGBA to grayscale float
    for (int y = 0; y < result.height; ++y) {
        for (int x = 0; x < result.width; ++x) {
            const size_t idx_a = (static_cast<size_t>(y) * frame_a.width + x) * frame_a.channels;
            const size_t idx_b = (static_cast<size_t>(y) * frame_b.width + x) * frame_b.channels;
            pixels_a[static_cast<size_t>(y) * result.width + x] = (
                0.299f * frame_a.data[idx_a] +
                0.587f * frame_a.data[idx_a + 1] +
                0.114f * frame_a.data[idx_a + 2]);
            pixels_b[static_cast<size_t>(y) * result.width + x] = (
                0.299f * frame_b.data[idx_b] +
                0.587f * frame_b.data[idx_b + 1] +
                0.114f * frame_b.data[idx_b + 2]);
        }
    }

    tracker::GrayImage gray_a, gray_b;
    gray_a.width = static_cast<uint32_t>(result.width);
    gray_a.height = static_cast<uint32_t>(result.height);
    gray_a.data = pixels_a.data();

    gray_b.width = static_cast<uint32_t>(result.width);
    gray_b.height = static_cast<uint32_t>(result.height);
    gray_b.data = pixels_b.data();

    tracker::FeatureTracker tracker;
    auto features = tracker.detect_features(gray_a);
    auto tracks = tracker.track_frame(gray_a, gray_b, features);

    // Simple dense flow: interpolate sparse tracks using inverse distance weighting (IDW)
    const float max_influence_dist = static_cast<float>(std::hypot(result.width, result.height)) * 0.2f;

    for (int y = 0; y < result.height; ++y) {
        for (int x = 0; x < result.width; ++x) {
            float w_sum = 0.0f;
            float dx_sum = 0.0f, dy_sum = 0.0f;

            for (std::size_t i = 0; i < tracks.size(); ++i) {
                const auto& track = tracks[i];
                if (track.confidence < 0.3f) continue;
                
                const float dx_track = track.position.x - features[i].x;
                const float dy_track = track.position.y - features[i].y;
                
                const float dist_x = static_cast<float>(x) - features[i].x;
                const float dist_y = static_cast<float>(y) - features[i].y;
                const float dist_sq = dist_x * dist_x + dist_y * dist_y + 1e-6f;
                const float dist = std::sqrt(dist_sq);

                if (dist < max_influence_dist) {
                    const float w = 1.0f / dist_sq;
                    dx_sum += dx_track * w;
                    dy_sum += dy_track * w;
                    w_sum += w;
                }
            }

            const size_t idx = static_cast<size_t>(y) * result.width + x;
            if (w_sum > 0) {
                result.flow_x[idx] = dx_sum / w_sum;
                result.flow_y[idx] = dy_sum / w_sum;
            } else {
                result.flow_x[idx] = 0;
                result.flow_y[idx] = 0;
            }
        }
    }

    return result;
}

} // namespace tachyon::timeline
