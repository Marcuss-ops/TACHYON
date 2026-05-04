#include "tachyon/color/blend_kernel.h"
#include "tachyon/color/blending.h"

namespace tachyon::color::kernel {

void normal(const LinearRGBA* src, LinearRGBA* dst, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        dst[i] = blend_normal(src[i], dst[i]);
    }
}

void additive(const LinearRGBA* src, LinearRGBA* dst, std::size_t count) {
    // Dispatch to SIMD if available (simplified for now)
    for (std::size_t i = 0; i < count; ++i) {
        dst[i] = blend_additive(src[i], dst[i]);
    }
}

void multiply(const LinearRGBA* src, LinearRGBA* dst, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        dst[i] = blend_multiply(src[i], dst[i]);
    }
}

void screen(const LinearRGBA* src, LinearRGBA* dst, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        dst[i] = blend_screen(src[i], dst[i]);
    }
}

} // namespace tachyon::color::kernel
