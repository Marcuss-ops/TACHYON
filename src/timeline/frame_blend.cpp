#include "tachyon/timeline/frame_blend.h"
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

} // namespace tachyon::timeline
