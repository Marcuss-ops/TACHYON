#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include <string>

namespace tachyon::renderer2d {

/**
 * @brief Attempts to apply an optimized fast-path for a transition.
 * 
 * Fast-paths bypass UV sampling and Color object construction when dimensions match.
 * 
 * @param transition_id The canonical ID of the transition.
 * @param output The surface to write into.
 * @param from The source surface.
 * @param to The target surface (optional).
 * @param progress Transition progress (0.0 to 1.0).
 * @param thread_count Number of threads to use for parallel execution.
 * @return true if a fast-path was applied, false if generic implementation should be used.
 */
bool apply_transition_fast_path(
    const std::string& transition_id,
    SurfaceRGBA& output,
    const SurfaceRGBA& from,
    const SurfaceRGBA* to,
    float progress,
    int thread_count);

void apply_soft_zoom_blur_fused_direct(
    SurfaceRGBA& output,
    const SurfaceRGBA& from,
    const SurfaceRGBA* to,
    float progress,
    int thread_count);


} // namespace tachyon::renderer2d
