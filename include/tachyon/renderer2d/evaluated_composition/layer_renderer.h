#pragma once

#include "tachyon/renderer2d/raster/rasterizer.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/renderer2d/evaluated_composition/render_intent.h"

#include <cstdint>
#include <memory>
#include <vector>
#include <optional>

namespace tachyon {
namespace renderer2d {

std::shared_ptr<SurfaceRGBA> render_precomp_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    const renderer2d::RenderIntent& intent,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext2D& context);

std::shared_ptr<SurfaceRGBA> render_simple_layer_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    const renderer2d::RenderIntent& intent,
    RenderContext2D& context,
    const std::optional<RectI>& target_rect = std::nullopt);

std::shared_ptr<SurfaceRGBA> render_layer_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    const renderer2d::RenderIntent& intent,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext2D& context,
    const std::optional<RectI>& target_rect = std::nullopt);

} // namespace renderer2d
} // namespace tachyon