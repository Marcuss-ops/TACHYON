#include "tachyon/color/blend_kernel.h"
#include "tachyon/color/blending.h"

namespace tachyon::color::kernel {

// Internal declarations for SIMD kernels
#ifdef _MSC_VER
    // We only enable this on MSVC for now as we have granular /arch:AVX2 set up there
    extern void additive_avx2(const LinearRGBA* src, LinearRGBA* dst, std::size_t count);
#endif

void normal(const LinearRGBA* src, LinearRGBA* dst, std::size_t count) {
    // Normal blending is complex for SIMD (AOS layout). 
    // We keep it scalar for correctness as per architectural review.
    for (std::size_t i = 0; i < count; ++i) {
        dst[i] = blend_normal(src[i], dst[i]);
    }
}

void additive(const LinearRGBA* src, LinearRGBA* dst, std::size_t count) {
#ifdef _MSC_VER
    additive_avx2(src, dst, count);
#else
    for (std::size_t i = 0; i < count; ++i) {
        dst[i] = blend_additive(src[i], dst[i]);
    }
#endif
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
