#include "tachyon/core/transition/transition_simd_kernels.h"

namespace tachyon::runtime::simd {

void lerp_pixels_scalar(float* out, const float* a, const float* b, std::size_t count, float t) {
    const float inv_t = 1.0f - t;
    for (std::size_t i = 0; i < count; ++i) {
        out[i] = a[i] * inv_t + b[i] * t;
    }
}

void lerp_pixels_best(float* out, const float* a, const float* b, std::size_t count, float t) {
#if defined(TACHYON_ENABLE_HIGHWAY)
    lerp_pixels_highway(out, a, b, count, t);
#else
    lerp_pixels_scalar(out, a, b, count, t);
#endif
}

} // namespace tachyon::runtime::simd
