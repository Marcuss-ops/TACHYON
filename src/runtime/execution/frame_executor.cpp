#include "frame_executor/frame_executor_internal.h"
#include "tachyon/core/scene/composition/evaluator_composition.h"
#include "tachyon/runtime/profiling/render_profiler.h"
#include "tachyon/renderer2d/raster/draw_list_builder.h"

#include <cstdlib>
#include <string_view>

namespace tachyon {

namespace {

std::optional<scene::EvaluatedCompositionState> evaluate_target_composition_state(
    const SceneSpec& scene,
    const RenderPlan& plan,
    const FrameRenderTask& task) {
    for (const auto& comp : scene.compositions) {
        if (comp.id != plan.composition_target) {
            continue;
        }

        double fps = comp.frame_rate.value();
        if (fps <= 0.0) {
            fps = 24.0;
        }

        const double time = static_cast<double>(task.frame_number) / fps;
        return scene::evaluate_composition_internal(
            &scene,
            comp,
            task.frame_number,
            time,
            {},
            nullptr,
            {},
            nullptr,
            std::nullopt,
            std::nullopt);
    }

    return std::nullopt;
}

} // namespace

void FrameExecutor::build_lookup_table(const CompiledScene& scene) {
    m_node_lookup.clear();
    m_property_lookup.clear();
    m_layer_lookup.clear();
    m_composition_lookup.clear();

    for (const auto& composition : scene.compositions) {
        m_node_lookup[composition.node.node_id] = &composition.node;
        m_composition_lookup[composition.node.node_id] = &composition;
        for (const auto& layer : composition.layers) {
            m_node_lookup[layer.node.node_id] = &layer.node;
            m_layer_lookup[layer.node.node_id] = &layer;
        }
    }
    for (const auto& track : scene.property_tracks) {
        m_node_lookup[track.node.node_id] = &track.node;
        m_property_lookup[track.node.node_id] = &track;
    }
}

ExecutedFrame FrameExecutor::execute(
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext& context) {
    auto snapshot = std::make_unique<DataSnapshot>();
    snapshot->scene_hash = compiled_scene.scene_hash;
    return execute(compiled_scene, plan, task, *snapshot, context);
}

ExecutedFrame FrameExecutor::execute(
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context) {
    profiling::ProfileScope frame_scope(context.profiler, profiling::ProfileEventType::Frame, "frame_render", task.frame_number);

    const char* diag_env = std::getenv("TACHYON_DIAGNOSTICS");
    const bool diagnostics_enabled = (diag_env && std::string_view(diag_env) == "1");
    context.diagnostic_tracker = nullptr;
    context.renderer2d.diagnostics = nullptr;
    context.policy = plan.quality_policy;
    context.renderer2d.policy = plan.quality_policy;

    if (m_node_lookup.empty()) {
        build_lookup_table(compiled_scene);
    }

    const FrameCacheState cache_state = build_frame_cache_state(compiled_scene, plan, task, diagnostics_enabled);
    const FrameTimingState timing_state = resolve_frame_timing(compiled_scene, plan, task);

    ExecutedFrame result;
    result.frame_number = task.frame_number;
    result.scene_hash = compiled_scene.scene_hash;
    result.cache_key = cache_state.frame_key;
    if (diagnostics_enabled) {
        result.diagnostics.composition_key_manifest = cache_state.composition_builder.manifest();
        result.diagnostics.frame_key_manifest = cache_state.frame_builder.manifest();
        context.diagnostic_tracker = &result.diagnostics;
        context.renderer2d.diagnostics = &result.diagnostics;
    }

    if (task.cacheable) {
        auto cached = m_cache.lookup_frame(cache_state.frame_key);
        if (cached) {
            result.frame = cached;
            result.cache_hit = true;
            return result;
        }
    }

    return run_frame_execution_pipeline(*this, compiled_scene, plan, task, snapshot, context, cache_state, timing_state);
}

// Delegate methods for backward compatibility
void FrameExecutor::evaluate_node(std::uint32_t node_id, const CompiledScene& scene, const RenderPlan& plan, const DataSnapshot& snapshot, RenderContext& context, std::uint64_t composition_key, std::uint64_t frame_key, double frame_time_seconds, const FrameRenderTask& task, std::optional<std::uint64_t> main_frame_key, std::optional<double> main_frame_time) {
    ::tachyon::evaluate_node(*this, node_id, scene, plan, snapshot, context, composition_key, frame_key, frame_time_seconds, task, main_frame_key, main_frame_time);
}
void FrameExecutor::evaluate_property(const CompiledScene& scene, const CompiledPropertyTrack& track, const RenderPlan& plan, const DataSnapshot& snapshot, RenderContext& context, std::uint64_t node_key, double frame_time_seconds) {
    ::tachyon::evaluate_property(*this, scene, track, plan, snapshot, context, node_key, frame_time_seconds);
}
void FrameExecutor::evaluate_layer(const CompiledScene& scene, const CompiledLayer& layer, const RenderPlan& plan, const DataSnapshot& snapshot, RenderContext& context, std::uint64_t composition_key, std::uint64_t frame_key, double frame_time_seconds, std::optional<std::uint64_t> main_frame_key, std::optional<double> main_frame_time) {
    ::tachyon::evaluate_layer(*this, scene, layer, plan, snapshot, context, composition_key, frame_key, frame_time_seconds, main_frame_key, main_frame_time);
}
void FrameExecutor::evaluate_composition(const CompiledScene& scene, const CompiledComposition& comp, const RenderPlan& plan, const DataSnapshot& snapshot, RenderContext& context, std::uint64_t composition_key, std::uint64_t node_key, std::uint64_t frame_key, double frame_time_seconds, const FrameRenderTask& task) {
    ::tachyon::evaluate_composition(*this, scene, comp, plan, snapshot, context, composition_key, node_key, frame_key, frame_time_seconds, task);
}

ExecutedFrame execute_frame_task(const SceneSpec& scene, const CompiledScene& compiled_scene, const RenderPlan& plan, const FrameRenderTask& task, FrameCache& cache, RenderContext& context) {
    (void)scene;
    FrameArena arena;
    FrameExecutor executor(arena, cache);
    auto snapshot = std::make_unique<DataSnapshot>();
    snapshot->scene_hash = compiled_scene.scene_hash;
    return executor.execute(compiled_scene, plan, task, *snapshot, context);
}

EvaluatedFrameState evaluate_frame_state(const SceneSpec& scene, const CompiledScene& compiled_scene, const RenderPlan& plan, const FrameRenderTask& task) {
    EvaluatedFrameState state;
    state.task = task;
    state.scene_hash = compiled_scene.scene_hash;
    if (const auto composition = evaluate_target_composition_state(scene, plan, task); composition.has_value()) {
        state.composition_state = std::move(*composition);
    }
    return state;
}

renderer2d::DrawList2D build_draw_list(const EvaluatedFrameState& state) {
    return renderer2d::DrawListBuilder::build(state.composition_state);
}

} // namespace tachyon
