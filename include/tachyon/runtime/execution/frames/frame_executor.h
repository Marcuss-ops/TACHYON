#pragma once

#include "tachyon/renderer2d/raster/draw_command.h"
#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include "tachyon/runtime/core/data/data_snapshot.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/runtime/frame_arena.h"
#include "tachyon/runtime/resource/runtime_surface_pool.h"

#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/output/frame_aov.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace tachyon {

/**
 * @brief Result of dynamic frame execution.
 */
struct ExecutedFrame {
    ExecutedFrame() = default;

    std::int64_t frame_number{0};
    std::uint64_t cache_key{0};
    bool cache_hit{false};
    bool success{true};
    std::string error;
    std::uint64_t scene_hash{0};
    std::size_t draw_command_count{0};
    std::shared_ptr<const renderer2d::Framebuffer> frame;
    std::vector<output::FrameAOV> aovs;
    FrameDiagnostics diagnostics;
};

struct EvaluatedFrameState {
    FrameRenderTask task;
    scene::EvaluatedCompositionState composition_state;
    std::uint64_t scene_hash{0};
};

/**
 * @brief High-performance frame execution engine.
 * 
 * The FrameExecutor consumes CompiledScene (the IR) and evaluates nodes 
 * according to the RenderGraph's topological order. It leverages 
 * incremental caching and an arena allocator for peak performance.
 */
class FrameExecutor {
public:
    explicit FrameExecutor(FrameArena& arena, FrameCache& cache, runtime::RuntimeSurfacePool* pool = nullptr)
        : m_arena(arena), m_cache(cache), m_pool(pool) {}

    void set_parallel_worker_count(size_t worker_count) {
        m_parallel_worker_count = worker_count;
        m_parallel_frames = (worker_count > 1);
    }

    size_t parallel_worker_count() const { return m_parallel_worker_count; }
    bool parallel_frames() const { return m_parallel_frames; }

    FrameCache& cache() { return m_cache; }
    const FrameCache& cache() const { return m_cache; }
    runtime::RuntimeSurfacePool* surface_pool() const { return m_pool; }

    /**
     * @brief Executes evaluation for a specific frame.
     */
    ExecutedFrame execute(
        const CompiledScene& compiled_scene,
        const RenderPlan& plan,
        const FrameRenderTask& task,
        RenderContext& context);

    ExecutedFrame execute(
        const CompiledScene& compiled_scene,
        const RenderPlan& plan,
        const FrameRenderTask& task,
        const DataSnapshot& snapshot,
        RenderContext& context);

private:
    FrameArena& m_arena;
    FrameCache& m_cache;
    runtime::RuntimeSurfacePool* m_pool{nullptr};
    size_t m_parallel_worker_count{1};
    bool m_parallel_frames{false};

    // Node lookup maps populated once per scene execution
    std::unordered_map<std::uint32_t, const CompiledNode*> m_node_lookup;
    std::unordered_map<std::uint32_t, const CompiledPropertyTrack*> m_property_lookup;
    std::unordered_map<std::uint32_t, const CompiledLayer*> m_layer_lookup;
    std::unordered_map<std::uint32_t, const CompiledComposition*> m_composition_lookup;

    void build_lookup_table(const CompiledScene& scene);

    // Internal evaluation helpers that work on CompiledNodes
    void evaluate_node(
        std::uint32_t node_id,
        const CompiledScene& scene,
        const RenderPlan& plan,
        const DataSnapshot& snapshot,
        RenderContext& context,
        std::uint64_t composition_key,
        std::uint64_t frame_key,
        double frame_time_seconds,
        const FrameRenderTask& task,
        std::optional<std::uint64_t> main_frame_key = std::nullopt,
        std::optional<double> main_frame_time = std::nullopt);

    void evaluate_property(
        const CompiledScene& scene,
        const CompiledPropertyTrack& track,
        const RenderPlan& plan,
        const DataSnapshot& snapshot,
        RenderContext& context,
        std::uint64_t node_key,
        double frame_time_seconds);

    void evaluate_layer(
        const CompiledScene& scene,
        const CompiledLayer& layer,
        const RenderPlan& plan,
        const DataSnapshot& snapshot,
        RenderContext& context,
        std::uint64_t composition_key,
        std::uint64_t frame_key,
        double frame_time_seconds,
        std::optional<std::uint64_t> main_frame_key = std::nullopt,
        std::optional<double> main_frame_time = std::nullopt);

    void evaluate_composition(
        const CompiledScene& scene,
        const CompiledComposition& comp,
        const RenderPlan& plan,
        const DataSnapshot& snapshot,
        RenderContext& context,
        std::uint64_t composition_key,
        std::uint64_t node_key,
        std::uint64_t frame_key,
        double frame_time_seconds,
        const FrameRenderTask& task);

    friend void evaluate_node(FrameExecutor&, std::uint32_t, const CompiledScene&, const RenderPlan&, const DataSnapshot&, RenderContext&, std::uint64_t, std::uint64_t, double, const FrameRenderTask&, std::optional<std::uint64_t>, std::optional<double>);
    friend void evaluate_property(FrameExecutor&, const CompiledScene&, const CompiledPropertyTrack&, const RenderPlan&, const DataSnapshot&, RenderContext&, std::uint64_t, double);
    friend void evaluate_layer(FrameExecutor&, const CompiledScene&, const CompiledLayer&, const RenderPlan&, const DataSnapshot&, RenderContext&, std::uint64_t, std::uint64_t, double, std::optional<std::uint64_t>, std::optional<double>);
    friend void evaluate_composition(FrameExecutor&, const CompiledScene&, const CompiledComposition&, const RenderPlan&, const DataSnapshot&, RenderContext&, std::uint64_t, std::uint64_t, std::uint64_t, double, const FrameRenderTask&, std::optional<std::uint64_t>, std::optional<double>);
};

[[nodiscard]] EvaluatedFrameState evaluate_frame_state(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task);

[[nodiscard]] renderer2d::DrawList2D build_draw_list(const EvaluatedFrameState& state);

// Legacy C-style wrappers for compatibility during transition
ExecutedFrame execute_frame_task(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    FrameCache& cache,
    RenderContext& context);

} // namespace tachyon
