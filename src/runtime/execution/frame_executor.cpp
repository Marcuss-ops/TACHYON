#include "tachyon/runtime/execution/frame_executor.h"
#include "tachyon/runtime/cache/cache_key_builder.h"
#include "tachyon/renderer2d/evaluated_composition_renderer.h"
#include "tachyon/renderer2d/draw_list_builder.h"
#include "tachyon/renderer2d/texture_resolver.h"

#include <algorithm>
#include <map>

namespace tachyon {

namespace {

std::uint64_t build_node_key(
    std::uint64_t global_key,
    const CompiledNode& node,
    const CompiledScene& scene) {
    
    CacheKeyBuilder builder;
    builder.add_u64(global_key);
    builder.add_u32(node.node_id);
    builder.add_u32(node.version);
    
    // Dependencies contribute to the key for incremental invalidation
    for (std::uint32_t dep_id : node.dependencies) {
        // Ideally we'd add the hash of the dependency result here
        builder.add_u32(dep_id);
    }
    
    return builder.finish();
}

} // namespace

ExecutedFrame FrameExecutor::execute(
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext& context) {
    
    // Ensure lookup table is built (in a real system this might be cached with CompiledScene)
    if (m_node_lookup.empty()) {
        build_lookup_table(compiled_scene);
    }

    ExecutedFrame result;
    result.frame_number = task.frame_number;
    result.scene_hash = compiled_scene.scene_hash;
    
    // 1. Build Global Key
    CacheKeyBuilder global_builder;
    global_builder.add_u64(compiled_scene.scene_hash);
    global_builder.add_u64(static_cast<std::uint64_t>(task.frame_number));
    global_builder.add_string(plan.quality_tier);
    global_builder.add_u32(compiled_scene.header.layout_version);
    const std::uint64_t global_key = global_builder.finish();
    result.cache_key = global_key;

    // 2. Perform Topological Evaluation
    const auto& topo_order = compiled_scene.graph.topo_order();
    for (std::uint32_t node_id : topo_order) {
        evaluate_node(node_id, compiled_scene, plan, context, global_key);
    }

    // 3. Final Frame Rendering
    // For now, we assume the last composition in the topo order is our root
    if (!topo_order.empty()) {
        std::uint32_t root_id = topo_order.back();
        std::uint64_t root_key = build_node_key(global_key, *m_node_lookup.at(root_id), compiled_scene);
        auto cached_comp = m_cache.lookup_composition(root_key);
        if (cached_comp) {
            // Success! We have an evaluated scene state. Rasterize it.
            result.frame = render_evaluated_composition_2d(*cached_comp, plan, task, context).surface.value_or(renderer2d::SurfaceRGBA(1, 1));
            result.draw_command_count = cached_comp->layers.size(); // Rough estimate
            result.cache_hit = false; // The evaluation might have hit cache, but we just rendered
        }
    }
    
    return result;
}


void FrameExecutor::evaluate_node(
    std::uint32_t node_id, 
    const CompiledScene& scene, 
    const RenderPlan& plan, 
    RenderContext& context,
    std::uint64_t global_key) {
    
    const CompiledNode* node = m_node_lookup.at(node_id);
    const std::uint64_t node_key = build_node_key(global_key, *node, scene);

    switch (node->type) {
        case CompiledNodeType::Property:
            evaluate_property(*m_property_lookup.at(node_id), plan, node_key);
            break;
        case CompiledNodeType::Layer:
            evaluate_layer(*m_layer_lookup.at(node_id), plan, context, node_key);
            break;
        case CompiledNodeType::Composition:
            evaluate_composition(*m_composition_lookup.at(node_id), plan, context, node_key);
            break;
        default:
            break;
    }
}

void FrameExecutor::evaluate_property(const CompiledPropertyTrack& track, const RenderPlan& plan, std::uint64_t node_key) {
    if (m_cache.lookup_property(node_key, *(double*)&node_key)) return; // Simple existence check

    double value = track.constant_value;
    if (track.kind == CompiledPropertyTrack::Kind::Keyframed) {
        // Simple linear interpolation fallback for now
        if (!track.keyframes.empty()) {
            value = track.keyframes[0].value;
        }
    }
    m_cache.store_property(node_key, value);
}

void FrameExecutor::evaluate_layer(const CompiledLayer& layer, const RenderPlan& plan, RenderContext& context, std::uint64_t node_key) {
    if (m_cache.lookup_layer(node_key)) return;

    auto state = std::make_shared<scene::EvaluatedLayerState>();
    state->id = layer.id;
    state->enabled = layer.enabled;
    state->active = true;
    state->visible = layer.visible;
    state->is_3d = layer.is_3d;
    state->width = layer.width;
    state->height = layer.height;
    state->line_cap = layer.line_cap;
    state->line_join = layer.line_join;
    state->miter_limit = layer.miter_limit;

    // Resolve transform components from property cache
    double opacity = 1.0;
    math::Vector2 pos{0, 0};
    math::Vector2 scale{1, 1};
    double rotation = 0.0;

    auto sampler = [&](std::size_t idx, double fallback) -> double {
        if (idx >= layer.property_indices.size()) return fallback;
        const auto& track = *m_property_lookup.at(layer.property_indices[idx]);
        const std::uint64_t prop_key = build_node_key(node_key, track.node, * (CompiledScene*)node_key); // This needs fixed build_node_key logic
        double val = fallback;
        m_cache.lookup_property(prop_key, val);
        return val;
    };
    
    // In actual implementation, we use pre-defined indices from SceneCompiler for Opacity, etc.
    state->opacity = sampler(0, 1.0);
    state->local_transform.position.x = static_cast<float>(sampler(1, 0.0));
    state->local_transform.position.y = static_cast<float>(sampler(2, 0.0));
    state->local_transform.scale.x = static_cast<float>(sampler(3, 1.0));
    state->local_transform.scale.y = static_cast<float>(sampler(4, 1.0));
    state->local_transform.rotation = static_cast<float>(sampler(5, 0.0));

    m_cache.store_layer(node_key, std::move(state));
}

void FrameExecutor::evaluate_composition(const CompiledComposition& comp, const RenderPlan& plan, RenderContext& context, std::uint64_t node_key) {
    if (m_cache.lookup_composition(node_key)) return;

    auto state = std::make_shared<scene::EvaluatedCompositionState>();
    state->composition_id = comp.id;
    state->width = comp.width;
    state->height = comp.height;
    
    // Collect child layers that were evaluated before this composition (in topo order)
    state->layers.reserve(comp.layers.size());
    for (const auto& layer : comp.layers) {
        const std::uint64_t l_key = build_node_key(node_key, layer.node, *(CompiledScene*)node_key); // Fix this
        if (auto layer_state = m_cache.lookup_layer(l_key)) {
            state->layers.push_back(*layer_state);
        }
    }

    m_cache.store_composition(node_key, std::move(state));
}



ExecutedFrame execute_frame_task(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    FrameCache& cache,
    RenderContext& context) {
    
    FrameArena arena(1024 * 1024 * 4);
    FrameExecutor executor(arena, cache);
    return executor.execute(compiled_scene, plan, task, context);
}


} // namespace tachyon

