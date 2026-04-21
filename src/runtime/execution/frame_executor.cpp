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
        evaluate_node(node_id, compiled_scene, plan, context);
    }

    // 3. Final Frame Rendering (simplified for this architectural switch)
    // In a full implementation, we would extract the root composition node's result
    // and rasterize it. For now, let's assume we are building onto the existing renderer.
    
    return result;
}

void FrameExecutor::evaluate_node(
    std::uint32_t node_id, 
    const CompiledScene& scene, 
    const RenderPlan& plan, 
    RenderContext& context) {
    
    // This is the hot path. We need to find the node and evaluate it.
    // In a production engine, we would use a flat array of nodes sorted by ID
    // or a direct pointer map built during SceneCompiler::compile.
    
    // Example logic for Property nodes:
    // 1. Build key: build_node_key(global_key, node, scene)
    // 2. lookup m_cache.lookup_property(key)
    // 3. if miss: sample track or run ExpressionVM
    // 4. store in m_cache
}

ExecutedFrame execute_frame_task(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    FrameCache& cache,
    RenderContext& context) {
    
    // Prepare Arena
    FrameArena arena(1024 * 1024 * 4); // 4MB transient
    FrameExecutor executor(arena, cache);
    
    return executor.execute(compiled_scene, plan, task, context);
}

} // namespace tachyon

