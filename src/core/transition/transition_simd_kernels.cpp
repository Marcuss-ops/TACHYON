#include "tachyon/core/transition/transition_simd_kernels.h"

namespace tachyon::runtime::simd {

void lerp_pixels_scalar(float* out, const float* a, const float* b, std::size_t count, float t) {
    const float inv_t = 1.0f - t;
    for (std::size_t i = 0; i < count; ++i) {
        out[i] = a[i] * inv_t + b[i] * t;
    }
}

} // namespace tachyon::runtime::simd
