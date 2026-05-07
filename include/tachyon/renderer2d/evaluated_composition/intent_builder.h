#pragma once

#include "tachyon/renderer2d/evaluated_composition/render_intent.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/renderer2d/resource/render_context.h"

namespace tachyon::renderer2d {

/**
 * @brief Utility to resolve logical asset IDs from evaluated state into physical resources.
 */
RenderIntent build_render_intent(
    const scene::EvaluatedCompositionState& state,
    const RenderContext2D& context);

} // namespace tachyon::renderer2d
