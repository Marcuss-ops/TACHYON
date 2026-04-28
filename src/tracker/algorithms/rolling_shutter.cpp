#include "tachyon/tracker/algorithms/rolling_shutter.h"
#include <cmath>
#include <algorithm>

namespace tachyon::tracker {

// Helper to multiply 3x3 matrices
static void mul33(const float a[9], const float b[9], float c[9]) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            c[i * 3 + j] = 0;
            for (int k = 0; k < 3; ++k) {
                c[i * 3 + k] += a[i * 3 + k] * b[k * 3 + j];
            }
        }
    }
}

// Helper to invert a 3x3 matrix
static bool inv33(const float m[9], float inv[9]) {
    float det = m[0] * (m[4] * m[8] - m[5] * m[7]) -
                m[1] * (m[3] * m[8] - m[5] * m[6]) +
                m[2] * (m[3] * m[7] - m[4] * m[6]);

    if (std::abs(det) < 1e-12f) return false;

    float inv_det = 1.0f / det;
    inv[0] = (m[4] * m[8] - m[5] * m[7]) * inv_det;
    inv[1] = (m[2] * m[7] - m[1] * m[8]) * inv_det;
    inv[2] = (m[1] * m[5] - m[2] * m[4]) * inv_det;
    inv[3] = (m[5] * m[6] - m[3] * m[8]) * inv_det;
    inv[4] = (m[0] * m[8] - m[2] * m[6]) * inv_det;
    inv[5] = (m[3] * m[2] - m[0] * m[5]) * inv_det;
    inv[6] = (m[3] * m[7] - m[4] * m[6]) * inv_det;
    inv[7] = (m[6] * m[1] - m[0] * m[7]) * inv_det;
    inv[8] = (m[0] * m[4] - m[1] * m[3]) * inv_det;

    return true;
}

void apply_rolling_shutter_correction(
    const std::vector<std::vector<float>>& homographies,
    const RollingShutterParams& params,
    std::vector<std::vector<float>>& frame_data,
    uint32_t width,
    uint32_t height) {

    if (frame_data.size() < 2 || homographies.size() != frame_data.size()) {
        return;
    }

    const float readout_ratio = params.readout_time_ms / params.frame_duration_ms;
    std::vector<std::vector<float>> corrected_frames(frame_data.size(), std::vector<float>(width * height));

    for (size_t i = 0; i < frame_data.size(); ++i) {
        // We need motion relative to frame i.
        // H(t) approximation:
        // For t in [i-0.5, i+0.5], we interpolate between H_{i-1}, H_i, H_{i+1}
        
        const float* Hi = homographies[i].data();
        const float* Hprev = (i > 0) ? homographies[i - 1].data() : Hi;
        const float* Hnext = (i < homographies.size() - 1) ? homographies[i + 1].data() : Hi;

        auto get_h = [&](float t_offset) -> std::vector<float> {
            std::vector<float> H(9);
            if (t_offset < 0) {
                float lerp = -t_offset;
                for (int j = 0; j < 9; ++j) H[j] = Hi[j] * (1.0f - lerp) + Hprev[j] * lerp;
            } else {
                float lerp = t_offset;
                for (int j = 0; j < 9; ++j) H[j] = Hi[j] * (1.0f - lerp) + Hnext[j] * lerp;
            }
            return H;
        };

        // Ideal pose is H(i)
        float Hi_inv[9];
        if (!inv33(Hi, Hi_inv)) {
            corrected_frames[i] = frame_data[i];
            continue;
        }

        for (uint32_t y = 0; y < height; ++y) {
            // Normalized y in [-0.5, 0.5]
            float ny = (float)y / (float)(height - 1) - 0.5f;
            if (params.scan_direction < 0) ny = -ny;

            float t_offset = ny * readout_ratio;
            std::vector<float> Ht = get_h(t_offset);

            // Warp W = Ht^-1 * Hi
            // Maps ideal pixel (at t=i) to captured pixel (at t=i+t_offset)
            float Ht_inv[9];
            if (!inv33(Ht.data(), Ht_inv)) {
                // Fallback: copy row
                for (uint32_t x = 0; x < width; ++x) {
                    corrected_frames[i][y * width + x] = frame_data[i][y * width + x];
                }
                continue;
            }

            float W[9];
            mul33(Ht_inv, Hi, W);

            for (uint32_t x = 0; x < width; ++x) {
                float sx = W[0] * x + W[1] * y + W[2];
                float sy = W[3] * x + W[4] * y + W[5];
                float sw = W[6] * x + W[7] * y + W[8];

                if (std::abs(sw) > 1e-6f) {
                    sx /= sw;
                    sy /= sw;
                }

                // Bilinear sample from frame_data[i]
                sx = std::clamp(sx, 0.0f, (float)width - 1.001f);
                sy = std::clamp(sy, 0.0f, (float)height - 1.001f);

                int ix = (int)sx;
                int iy = (int)sy;
                float tx = sx - ix;
                float ty = sy - iy;

                float v00 = frame_data[i][iy * width + ix];
                float v10 = frame_data[i][iy * width + ix + 1];
                float v01 = frame_data[i][(iy + 1) * width + ix];
                float v11 = frame_data[i][(iy + 1) * width + ix + 1];

                float val = (v00 * (1.0f - tx) + v10 * tx) * (1.0f - ty) +
                            (v01 * (1.0f - tx) + v11 * tx) * ty;

                corrected_frames[i][y * width + x] = val;
            }
        }
    }

    frame_data = std::move(corrected_frames);
}

} // namespace tachyon::tracker

