#include "tachyon/runtime/execution/frame_executor/frame_pipeline_steps.h"
#include "frame_executor_internal.h"
#include "tachyon/runtime/execution/frame_blend_renderer.h"
#include "tachyon/runtime/execution/motion_blur_sampler.h"
#include "tachyon/runtime/execution/frame_fallback_policy.h"
#include "tachyon/runtime/execution/rasterization_step.h"
#include "tachyon/renderer2d/evaluated_composition/intent_builder.h"

namespace tachyon {

std::optional<ExecutedFrame> try_frame_blend_step(
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context,
    const FrameCacheState& cache_state,
    const FrameTimingState& timing_state) {

    auto blended_frame = FrameBlendRenderer::try_render_blend(
        executor,
        compiled_scene,
        plan,
        task,
        snapshot,
        context,
        timing_state.blend_result,
        timing_state.frame_time_seconds,
        cache_state.composition_key,
        cache_state.frame_key);

    if (blended_frame.has_value()) {
        ExecutedFrame result;
        result.frame_number = task.frame_number;
        result.scene_hash = compiled_scene.scene_hash;
        result.frame = *blended_frame;
        result.cache_hit = false;
        return result;
    }

    return std::nullopt;
}

std::optional<ExecutedFrame> try_motion_blur_step(
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context,
    const FrameCacheState& cache_state,
    const FrameTimingState& timing_state) {

    auto mb_frame = MotionBlurSampler::sample(
        executor,
        compiled_scene,
        plan,
        task,
        snapshot,
        context,
        timing_state.frame_time_seconds,
        cache_state.composition_key,
        cache_state.frame_key,
        static_cast<int>(timing_state.fps),
        cache_state.composition_builder);

    if (mb_frame.has_value()) {
        ExecutedFrame result;
        result.frame_number = task.frame_number;
        result.scene_hash = compiled_scene.scene_hash;
        result.frame = *mb_frame;
        result.cache_hit = false;
        return result;
    }

    return std::nullopt;
}

void evaluate_frame_graph_step(
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context,
    const FrameCacheState& cache_state,
    const FrameTimingState& timing_state) {

    const auto& topo_order = compiled_scene.graph.topo_order();
    for (std::uint32_t node_id : topo_order) {
        if (context.cancel_flag && context.cancel_flag->load()) {
            break;
        }
        ::tachyon::evaluate_node(
            executor,
            node_id,
            compiled_scene,
            plan,
            snapshot,
            context,
            cache_state.composition_key,
            cache_state.frame_key,
            timing_state.frame_time_seconds,
            task);
    }
}

void evaluate_and_rasterize_root_composition_step(
    ExecutedFrame& result,
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context,
    const FrameCacheState& cache_state,
    const FrameTimingState& timing_state) {

    if (!compiled_scene.compositions.empty()) {
        const CompiledComposition& root_comp = compiled_scene.compositions.front();
        const std::uint64_t root_key = build_node_key(cache_state.frame_key, root_comp.node);
        ::tachyon::evaluate_composition(
            executor,
            compiled_scene,
            root_comp,
            plan,
            snapshot,
            context,
            cache_state.composition_key,
            root_key,
            cache_state.frame_key,
            timing_state.frame_time_seconds,
            task);

        auto cached_comp = executor.cache().lookup_composition(root_key);
        if (cached_comp) {
            const auto intent = renderer2d::build_render_intent(*cached_comp, context.renderer2d);
            RasterizationResult raster_result = RasterizationStep::execute(
                *cached_comp,
                intent,
                plan,
                task,
                context,
                executor.surface_pool(),
                context.profiler,
                task.frame_number);
            result.frame = raster_result.frame;
            result.aovs = std::move(raster_result.aovs);
            result.draw_command_count = raster_result.draw_command_count;
        }
    }
}

void finalize_frame_step(
    ExecutedFrame& result,
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext& context,
    const FrameCacheState& cache_state) {

    FrameFallbackPolicy::apply(result, compiled_scene, plan);

    if (task.cacheable && result.frame) {
        executor.cache().store_frame(cache_state.frame_key, result.frame);
    }

    context.diagnostic_tracker = nullptr;
    context.renderer2d.diagnostics = nullptr;
}

} // namespace tachyon
