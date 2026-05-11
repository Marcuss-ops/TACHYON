#include "tachyon/core/simd/conversion.h"

#if defined(TACHYON_ENABLE_HIGHWAY)
#include <hwy/highway.h>

namespace tachyon::core::simd {
namespace hn = hwy::HWY_NAMESPACE;

namespace {

void rgba8_to_float32_highway(float* dst, const std::uint8_t* src, std::size_t pixel_count) {
    const hn::ScalableTag<float> df;
    const hn::ScalableTag<uint32_t> d32;
    
    const std::size_t total_components = pixel_count * 4;
    const auto lanes = hn::Lanes(df);
    
    const auto inv_255 = hn::Set(df, 1.0f / 255.0f);
    
    std::size_t i = 0;

    for (; i + lanes <= total_components; i += lanes) {
        using D8 = hn::Rebind<uint8_t, decltype(df)>;
        using D16 = hn::Rebind<uint16_t, decltype(df)>;
        const D8 d8;
        const D16 d16;
        
        const auto v8 = hn::LoadU(d8, src + i);
        const auto v16 = hn::PromoteTo(d16, v8);
        const auto v32 = hn::PromoteTo(d32, v16);
        const auto vf = hn::ConvertTo(df, v32);
        
        const auto result = hn::Mul(vf, inv_255);
        hn::StoreU(result, df, dst + i);
    }

    for (; i < total_components; ++i) {
        dst[i] = static_cast<float>(src[i]) / 255.0f;
    }
}

} // namespace

void rgba8_to_float32(float* dst, const std::uint8_t* src, std::size_t pixel_count) {
    rgba8_to_float32_highway(dst, src, pixel_count);
}

} // namespace tachyon::core::simd

#else

namespace tachyon::core::simd {

void rgba8_to_float32(float* dst, const std::uint8_t* src, std::size_t pixel_count) {
    const std::size_t total_components = pixel_count * 4;
    for (std::size_t i = 0; i < total_components; ++i) {
        dst[i] = static_cast<float>(src[i]) / 255.0f;
    }
}

} // namespace tachyon::core::simd

#endif
