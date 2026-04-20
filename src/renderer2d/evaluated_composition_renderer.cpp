#include "tachyon/renderer2d/evaluated_composition_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/evaluated_composition_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/mask_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/effect_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/render_graph.h"
#include "tachyon/renderer2d/render_context.h"

namespace tachyon {

RasterizedFrame2D render_evaluated_composition_2d(
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    renderer2d::RenderContext& context) {
    (void)plan;
    
    // 1. Prepare accumulation buffers
    context.accumulation_buffer.resize(static_cast<std::size_t>(state.width) * state.height);
    
    // 2. Build execution order (RenderGraph)
    renderer2d::RenderGraph graph(state);
    graph.buildDependencyGraph();
    auto execution_order = graph.resolveExecutionOrder();
    (void)execution_order;
    
    // 3. Render layers
    for (const auto& layer : state.layers) {
        renderer2d::LayerRenderer::renderLayer(layer, state, context, 
            context.accumulation_buffer.r, 
            context.accumulation_buffer.g, 
            context.accumulation_buffer.b, 
            context.accumulation_buffer.a);
    }
    
    // 4. Apply effects/masks if any (simplified here)
    renderer2d::MaskRenderer::applyMask(state, context, 
        context.accumulation_buffer.r, context.accumulation_buffer.g, 
        context.accumulation_buffer.b, context.accumulation_buffer.a);
        
    if (context.effects) {
        renderer2d::EffectRenderer::applyEffects(*context.effects, context, 
            context.accumulation_buffer.r, context.accumulation_buffer.g, 
            context.accumulation_buffer.b, context.accumulation_buffer.a);
    }
    
    // 5. Finalize frame
    RasterizedFrame2D frame;
    frame.frame_number = task.frame_number;
    frame.width = state.width;
    frame.height = state.height;
    // ... finalize surface ...
    
    return frame;
}

} // namespace tachyon
