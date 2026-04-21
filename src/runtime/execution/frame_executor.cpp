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
            // Success! We have a result. 
            // In a full implementation we'd then rasterize cached_comp to result.frame
            result.cache_hit = true;
        }
    }
    
    return result;
}

void FrameExecutor::build_lookup_table(const CompiledScene& scene) {
    for (const auto& comp : scene.compositions) {
        m_node_lookup[comp.node.node_id] = &comp.node;
        m_composition_lookup[comp.node.node_id] = &comp;
        for (const auto& layer : comp.layers) {
            m_node_lookup[layer.node.node_id] = &layer.node;
            m_layer_lookup[layer.node.node_id] = &layer;
        }
    }
    for (const auto& track : scene.property_tracks) {
        m_node_lookup[track.node.node_id] = &track.node;
        m_property_lookup[track.node.node_id] = &track;
    }
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
            if (!m_cache.lookup_property(node_key, *(double*)&global_key)) { // Dummy lookup to check exist
                 evaluate_property(*m_property_lookup.at(node_id), plan, node_key);
            }
            break;
        case CompiledNodeType::Layer:
            if (!m_cache.lookup_layer(node_key)) {
                evaluate_layer(*m_layer_lookup.at(node_id), plan, context, node_key);
            }
            break;
        case CompiledNodeType::Composition:
            if (!m_cache.lookup_composition(node_key)) {
                evaluate_composition(*m_composition_lookup.at(node_id), plan, context, node_key);
            }
            break;
        default:
            break;
    }
}

void FrameExecutor::evaluate_property(const CompiledPropertyTrack& track, const RenderPlan& plan, std::uint64_t node_key) {
    double value = track.constant_value;
    // ... animation sampling logic ...
    m_cache.store_property(node_key, value);
}

void FrameExecutor::evaluate_layer(const CompiledLayer& layer, const RenderPlan& plan, RenderContext& context, std::uint64_t node_key) {
    auto state = std::make_shared<scene::EvaluatedLayerState>();
    state->id = layer.id;
    state->name = layer.name;
    // ... resolve properties from cache and compute transform ...
    m_cache.store_layer(node_key, std::move(state));
}

void FrameExecutor::evaluate_composition(const CompiledComposition& comp, const RenderPlan& plan, RenderContext& context, std::uint64_t node_key) {
    auto state = std::make_shared<scene::EvaluatedCompositionState>();
    state->composition_id = comp.id;
    state->composition_name = comp.name;
    // ... collect layer results from cache ...
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

