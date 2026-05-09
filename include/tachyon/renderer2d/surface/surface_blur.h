#pragma once
#include "tachyon/renderer2d/core/framebuffer.h"

namespace tachyon::renderer2d {

/**
 * High-performance separable box blur.
 * @param surface The surface to blur (modified in-place or into a temp).
 * @param radius Radius of the blur in pixels.
 * @param thread_count Number of threads to use.
 */
void apply_box_blur(SurfaceRGBA& surface, int radius, int thread_count = 0);

/**
 * High-performance separable gaussian-like blur (via multiple box passes).
 * @param surface The surface to blur.
 * @param sigma Standard deviation (radius approximation).
 * @param passes Number of box blur passes (3 is close to Gaussian).
 */
void apply_fast_gaussian_blur(SurfaceRGBA& surface, float sigma, int passes = 3, int thread_count = 0);

} // namespace tachyon::renderer2d
