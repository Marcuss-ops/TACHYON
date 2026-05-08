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

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < static_cast<int>(result.height()); ++y) {
        if (cancel_flag && cancel_flag->load()) continue;
        for (std::uint32_t x = 0; x < result.width(); ++x) {
            const float u = (static_cast<float>(x) + offset_x + 0.5f) / layer_w;
            const float v = (static_cast<float>(y) + offset_y + 0.5f) / layer_h;
            const Color out = transition_fn(u, v, progress, from, to);
            result.set_pixel(x, static_cast<std::uint32_t>(y), out);
        }
    }
}

} // namespace tachyon::renderer2d
