#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"

#include "layer_renderer_shapes.h"
#include "layer_renderer_glyph.h"
#include "layer_renderer_text.h"
#include "layer_renderer_mask.h"
#include "layer_renderer_simple.h"

namespace tachyon::renderer2d {

std::shared_ptr<SurfaceRGBA> render_layer_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext2D& context,
    const std::optional<RectI>& target_rect) {

    if (layer.type == scene::LayerType::Precomp) {
        return render_precomp_surface(layer, state, plan, task, context);
    } else {
        return render_simple_layer_surface(layer, state, context, target_rect);
    }
}

} // namespace tachyon::renderer2d
