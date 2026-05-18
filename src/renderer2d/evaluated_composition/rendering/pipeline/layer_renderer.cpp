#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"

#include "../primitives/layer_renderer_shapes.h"
#include "../primitives/layer_renderer_glyph.h"
#include "../primitives/layer_renderer_text.h"
#include "../primitives/layer_renderer_mask.h"
#include "../primitives/layer_renderer_simple.h"

#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/runtime/cache/node_cache.h"

namespace tachyon::renderer2d {

std::shared_ptr<SurfaceRGBA> render_layer_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    const render::RenderIntent& intent,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext& context,
    const std::optional<RectI>& target_rect) {

    bool use_cache = (layer.temporal_stability == TemporalStability::Static) && context.node_cache;
    NodeCacheKey key;

    if (use_cache) {
        key.scene_hash = plan.scene_hash;
        try {
            key.node_id = std::stoull(layer.identity.layer_id);
        } catch (...) {
            key.node_id = 0;
        }
        key.dependency_hash = 0;
        key.width = static_cast<int>(layer.transform.width);
        key.height = static_cast<int>(layer.transform.height);
        key.time_bucket = 0.0;
        key.color_space = context.working_color_space.profile.to_string();
        key.quality = plan.quality_tier;

        auto cached = context.node_cache->lookup(key);
        if (cached) {
            if (context.diagnostics) {
                context.diagnostics->node_cache_lookups++;
                context.diagnostics->node_cache_hits++;
                context.diagnostics->static_nodes_detected++;
            }
            return std::make_shared<SurfaceRGBA>(*cached);
        }

        if (context.diagnostics) {
            context.diagnostics->node_cache_lookups++;
            context.diagnostics->node_cache_misses++;
            context.diagnostics->static_nodes_detected++;
        }
    } else {
        if (context.diagnostics) {
            context.diagnostics->animated_nodes_detected++;
        }
    }

    std::shared_ptr<SurfaceRGBA> result;
    if (layer.identity.type == LayerType::Precomp) {
        result = render_precomp_surface(layer, state, intent, plan, task, context);
    } else {
        result = render_simple_layer_surface(layer, state, intent, context, target_rect);
    }

    if (use_cache && result) {
        context.node_cache->store(key, result);
        if (context.diagnostics) {
            context.diagnostics->node_cache_bytes = context.node_cache->current_usage_bytes();
        }
    }

    return result;
}

} // namespace tachyon::renderer2d

