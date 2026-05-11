#pragma once

#include "tachyon/core/api.h"
#include <vector>
#include <cstdint>

namespace tachyon::timeline {

struct FrameBuffer {
    std::vector<uint8_t> data;
    int width{0};
    int height{0};
    int channels{4}; // RGBA
};

/**
 * @brief Blend two frames linearly based on blend factor
 */
TACHYON_API FrameBuffer blend_linear(const FrameBuffer& a, const FrameBuffer& b, float factor);

} // namespace tachyon::timeline
