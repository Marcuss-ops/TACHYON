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

// output_surface: optional pre-allocated surface to render into directly.
// When provided and dimensions match the working resolution, no internal
// surface allocation occurs and the caller's surface is returned as-is —
// eliminating the final blit in RasterizationStep.
RasterizedFrame2D render_evaluated_composition_2d(
    const scene::EvaluatedCompositionState& state,
    const render::RenderIntent& intent,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext& context,
    const renderer2d::EffectRegistry& effect_registry,
    std::shared_ptr<renderer2d::SurfaceRGBA> output_surface = nullptr);

} // namespace tachyon
