#pragma once

#include "tachyon/renderer2d/raster/rasterizer.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include <vector>

namespace tachyon {

std::vector<renderer2d::DrawCommand2D> build_draw_commands_from_evaluated_state(
    const scene::EvaluatedCompositionState& state);

namespace renderer2d { class EffectRegistry; }
namespace render { struct RenderIntent; }

RasterizedFrame2D render_evaluated_composition_2d(
    const scene::EvaluatedCompositionState& state,
    const render::RenderIntent& intent,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext& context,
    const renderer2d::EffectRegistry& effect_registry);

} // namespace tachyon
