#pragma once
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/core/transition/transition_effect_resolver.h"

namespace tachyon::renderer2d {

/**
 * Executes a pixel-based transition at a lower resolution and upsamples the result.
 * This is a general optimization for heavy effects like Soft Zoom Blur or Ripple.
 */
bool apply_transition_downsampled(
    CpuTransitionFn cpu_fn,
    SurfaceRGBA& output,
    const SurfaceRGBA& from,
    const SurfaceRGBA* to,
    float progress,
    int downsample_factor,
    int thread_count = 0
);

} // namespace tachyon::renderer2d
