#include "simd_transition_renderers.h"
#include "simd_kernels_internal.h"
#include "tachyon/core/transition/transition_simd_kernels.h"
#include "tachyon/tachyon_build_config.h"

namespace tachyon::backends::simd {


#if defined(TACHYON_ENABLE_HIGHWAY)
core::MediaResult<renderer2d::SurfaceRGBA> HighwayTransitionRenderer::render(
    const renderer2d::SurfaceRGBA& from,
    const renderer2d::SurfaceRGBA* to,
    float progress
) {
    if (!to) return core::MediaResult<renderer2d::SurfaceRGBA>::success(from);

    renderer2d::SurfaceRGBA out(from.width(), from.height());
    std::size_t pixel_count = from.width() * from.height();

    tachyon::runtime::simd::lerp_pixels_highway(
        out.data(),
        from.data(),
        to->data(),
        pixel_count * 4,
        progress
    );

    return core::MediaResult<renderer2d::SurfaceRGBA>::success(std::move(out));
}
#endif

} // namespace tachyon::backends::simd
