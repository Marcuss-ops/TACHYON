#include <cstdint>
#include <vector>

#if defined(TACHYON_ENABLE_HIGHWAY)
#include <hwy/highway.h>

namespace tachyon::media::simd {
namespace hn = hwy::HWY_NAMESPACE;

namespace {

// Optimized copy from raw RGBA buffer to basisu::image-like layout
void copy_rgba_to_basisu_impl(uint8_t* HWY_RESTRICT dst, const uint8_t* HWY_RESTRICT src, std::size_t num_pixels) {
    const hn::ScalableTag<uint8_t> d;
    const std::size_t lanes = hn::Lanes(d);

    std::size_t i = 0;
    std::size_t count = num_pixels * 4;
    for (; i + lanes <= count; i += lanes) {
        const auto v = hn::LoadU(d, src + i);
        hn::StoreU(v, d, dst + i);
    }
    for (; i < count; ++i) {
        dst[i] = src[i];
    }
}

} // namespace

void copy_rgba_to_basisu(uint8_t* dst, const uint8_t* src, std::size_t num_pixels) {
    copy_rgba_to_basisu_impl(dst, src, num_pixels);
}

} // namespace tachyon::media::simd

#endif
