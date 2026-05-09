#include "tachyon/renderer2d/effects/core/transitions/transition_downsampler.h"
#include "tachyon/renderer2d/surface/surface_sampling.h"
#include <algorithm>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace tachyon::renderer2d {

namespace {

SurfaceRGBA downsample(const SurfaceRGBA& src, int factor) {
    uint32_t nw = std::max(1u, src.width() / factor);
    uint32_t nh = std::max(1u, src.height() / factor);
    SurfaceRGBA dst(nw, nh);
    dst.set_profile(src.profile());
    
    // We could use bilinear for downsampling too, but nearest is faster 
    // and for heavy blur effects the difference is minimal.
    for (uint32_t y = 0; y < nh; ++y) {
        for (uint32_t x = 0; x < nw; ++x) {
            dst.set_pixel(x, y, src.get_pixel(x * factor, y * factor));
        }
    }
    return dst;
}

void upsample_bilinear(SurfaceRGBA& dst, const SurfaceRGBA& src) {
    uint32_t dw = dst.width();
    uint32_t dh = dst.height();
    
    #ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic)
    #endif
    for (int y = 0; y < static_cast<int>(dh); ++y) {
        for (uint32_t x = 0; x < dw; ++x) {
            float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(dw);
            float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(dh);
            dst.set_pixel(x, y, sample_texture_bilinear(src, u, v, Color::white()));
        }
    }
}

} // namespace

bool apply_transition_downsampled(
    CpuTransitionFn cpu_fn,
    SurfaceRGBA& output,
    const SurfaceRGBA& from,
    const SurfaceRGBA* to,
    float progress,
    int downsample_factor,
    int thread_count
) {
    if (!cpu_fn) return false;
    if (downsample_factor <= 1) return false;

    // 1. Create small surfaces
    SurfaceRGBA small_from = downsample(from, downsample_factor);
    SurfaceRGBA small_to_buf;
    const SurfaceRGBA* small_to = nullptr;
    if (to) {
        small_to_buf = downsample(*to, downsample_factor);
        small_to = &small_to_buf;
    }

    uint32_t sw = small_from.width();
    uint32_t sh = small_from.height();
    SurfaceRGBA small_out(sw, sh);
    small_out.set_profile(output.profile());

    // 2. Run the heavy kernel at low res
    int omp_threads = thread_count > 0 ? thread_count : 1;
    #ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic) num_threads(omp_threads)
    #endif
    for (int y = 0; y < static_cast<int>(sh); ++y) {
        for (uint32_t x = 0; x < sw; ++x) {
            float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(sw);
            float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(sh);
            // The kernel receives small surfaces, so u,v logic remains consistent
            Color c = cpu_fn(u, v, progress, small_from, small_to);
            small_out.set_pixel(x, y, c);
        }
    }

    // 3. Upsample back to full res
    upsample_bilinear(output, small_out);

    return true;
}

} // namespace tachyon::renderer2d
