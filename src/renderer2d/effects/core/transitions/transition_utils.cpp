#include "tachyon/renderer2d/effects/core/transitions/transition_utils.h"
#include "tachyon/core/transition/transition_descriptor.h"

namespace tachyon::renderer2d {

void apply_pixel_transition(
    SurfaceRGBA& result,
    const SurfaceRGBA& from,
    const SurfaceRGBA* to,
    CpuTransitionFn transition_fn,
    float progress,
    float layer_w,
    float layer_h,
    float offset_x,
    float offset_y,
    const std::atomic<bool>* cancel_flag) 
{
    if (!transition_fn) return;

    auto& pixels = result.mutable_pixels();
    const std::size_t stride = static_cast<std::size_t>(result.width()) * 4;

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < static_cast<int>(result.height()); ++y) {
        if (cancel_flag && cancel_flag->load()) continue;
        const std::size_t row_start = static_cast<std::size_t>(y) * stride;
        for (std::uint32_t x = 0; x < result.width(); ++x) {
            const float u = (static_cast<float>(x) + offset_x + 0.5f) / layer_w;
            const float v = (static_cast<float>(y) + offset_y + 0.5f) / layer_h;
            const Color out = transition_fn(u, v, progress, from, to);
            
            const std::size_t idx = row_start + (static_cast<std::size_t>(x) * 4);
            pixels[idx] = out.r;
            pixels[idx + 1] = out.g;
            pixels[idx + 2] = out.b;
            pixels[idx + 3] = out.a;
        }
    }
}

} // namespace tachyon::renderer2d
