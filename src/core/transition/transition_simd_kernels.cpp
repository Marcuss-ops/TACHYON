#include "tachyon/core/transition/transition_simd_kernels.h"

namespace tachyon::runtime::simd {

void lerp_pixels_scalar(float* out, const float* a, const float* b, std::size_t count, float t) {
    const float inv_t = 1.0f - t;
    for (std::size_t i = 0; i < count; ++i) {
        out[i] = a[i] * inv_t + b[i] * t;
    }
}

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)

#if defined(__GNUC__) || defined(__clang__)

bool avx2_available() {
    static const bool supported = __builtin_cpu_supports("avx2");
    return supported;
}

#elif defined(_MSC_VER)
#include <intrin.h>

bool avx2_available() {
    int info[4] = {0};
    __cpuidex(info, 7, 0);
    return (info[1] & (1U << 5)) != 0;
}
#endif

#else
// Unknown architecture — assume no AVX2
bool avx2_available() {
    return false;
}
#endif

void lerp_pixels_best(float* out, const float* a, const float* b, std::size_t count, float t) {
#if defined(TACHYON_ENABLE_HIGHWAY)
    lerp_pixels_highway(out, a, b, count, t);
    return;
#elif defined(TACHYON_AVX2)
    // AVX2 is compiled in and available at runtime
    if (avx2_available()) {
        lerp_pixels_avx2(out, a, b, count, t);
        return;
    }
#endif
    lerp_pixels_scalar(out, a, b, count, t);
}

} // namespace tachyon::runtime::simd