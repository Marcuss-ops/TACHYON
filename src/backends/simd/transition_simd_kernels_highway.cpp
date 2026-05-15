#include "tachyon/core/transition/transition_simd_kernels.h"

#if defined(TACHYON_ENABLE_HIGHWAY)

#include <hwy/highway.h>

namespace tachyon::runtime::simd {
namespace hn = hwy::HWY_NAMESPACE;

namespace {

void lerp_pixels_highway_impl(float* out, const float* a, const float* b, std::size_t count, float t) {
    const hn::ScalableTag<float> d;
    const auto lanes = hn::Lanes(d);

    const auto vt = hn::Set(d, t);
    const auto vinv = hn::Set(d, 1.0f - t);

    std::size_t i = 0;

    for (; i + lanes <= count; i += lanes) {
        const auto va = hn::LoadU(d, a + i);
        const auto vb = hn::LoadU(d, b + i);
        const auto result = hn::MulAdd(vb, vt, hn::Mul(va, vinv));
        hn::StoreU(result, d, out + i);
    }

    for (; i < count; ++i) {
        out[i] = a[i] * (1.0f - t) + b[i] * t;
    }
}

} // namespace

void lerp_pixels_highway(float* out, const float* a, const float* b, std::size_t count, float t) {
    lerp_pixels_highway_impl(out, a, b, count, t);
}

} // namespace tachyon::runtime::simd

#endif
