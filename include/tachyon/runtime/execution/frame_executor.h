#pragma once

#include "tachyon/renderer2d/draw_command.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include "tachyon/runtime/execution/render_plan.h"
#include "tachyon/runtime/core/compiled_scene.h"
#include "tachyon/runtime/frame_arena.h"

#include <cstddef>
#include <string>
#include <vector>

namespace tachyon {

/**
 * @brief High-performance frame execution engine.
 * 
 * The FrameExecutor consumes CompiledScene (the IR) and evaluates nodes 
 * according to the RenderGraph's topological order. It leverages 
 * incremental caching and an arena allocator for peak performance.
 */
class FrameExecutor {
public:
    explicit FrameExecutor(FrameArena& arena, FrameCache& cache)
        : m_arena(arena), m_cache(cache) {}

    /**
     * @brief Executes evaluation for a specific frame.
     */
    ExecutedFrame execute(
        const CompiledScene& compiled_scene,
        const RenderPlan& plan,
        const FrameRenderTask& task,
        RenderContext& context);

private:
    FrameArena& m_arena;
    FrameCache& m_cache;

    // Node lookup maps populated once per scene execution
    std::unordered_map<std::uint32_t, const CompiledNode*> m_node_lookup;
    std::unordered_map<std::uint32_t, const CompiledPropertyTrack*> m_property_lookup;
    std::unordered_map<std::uint32_t, const CompiledLayer*> m_layer_lookup;
    std::unordered_map<std::uint32_t, const CompiledComposition*> m_composition_lookup;

    void build_lookup_table(const CompiledScene& scene);

    // Internal evaluation helpers that work on CompiledNodes
    void evaluate_node(std::uint32_t node_id, const CompiledScene& scene, const RenderPlan& plan, RenderContext& context, std::uint64_t global_key);
    
    void evaluate_property(const CompiledPropertyTrack& track, const RenderPlan& plan, std::uint64_t node_key);
    void evaluate_layer(const CompiledLayer& layer, const RenderPlan& plan, RenderContext& context, std::uint64_t node_key);
    void evaluate_composition(const CompiledComposition& comp, const RenderPlan& plan, RenderContext& context, std::uint64_t node_key);
};


/**
 * @brief Evaluated state of a single frame (transient).
 */
struct EvaluatedFrameState {
    FrameRenderTask task;
    scene::EvaluatedCompositionState composition_state;
    std::uint64_t scene_hash{0};
};

/**
 * @brief Result of dynamic frame execution.
 */
struct ExecutedFrame {
    std::int64_t frame_number{0};
    std::uint64_t cache_key{0};
    bool cache_hit{false};
    std::uint64_t scene_hash{0};
    std::size_t draw_command_count{0};
    renderer2d::Framebuffer frame{1, 1};
};

// Legacy C-style wrappers for compatibility during transition
ExecutedFrame execute_frame_task(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    FrameCache& cache,
    RenderContext& context);

} // namespace tachyon

