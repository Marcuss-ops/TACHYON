#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/renderer2d/core/framebuffer.h"

namespace tachyon::renderer2d {

/**
 * @brief Render a procedural layer to a framebuffer.
 * 
 * Generates procedural patterns (noise, waves, stripes, gradient_sweep)
 * based on the ProceduralSpec parameters sampled at the current frame.
 * 
 * @param fb Framebuffer to render to
 * @param evaluated The evaluated layer state containing procedural spec
 * @param time_seconds Current time in seconds
 */
void render_procedural_layer(
    Framebuffer& fb,
    const scene::EvaluatedLayerState& evaluated,
    double time_seconds);

} // namespace tachyon::renderer2d
