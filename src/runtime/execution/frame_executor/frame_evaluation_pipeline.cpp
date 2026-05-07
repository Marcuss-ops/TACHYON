#include "tachyon/runtime/execution/frame_executor/frame_pipeline_steps.h"

namespace tachyon {

ExecutedFrame run_frame_execution_pipeline(
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context,
    const FrameCacheState& cache_state,
    const FrameTimingState& timing_state) {

    // 1. Try to return a blended frame (time-stretching)
    if (auto result = try_frame_blend_step(
            executor, compiled_scene, plan, task, snapshot, context, cache_state, timing_state)) {
        finalize_frame_step(*result, executor, compiled_scene, plan, task, context, cache_state);
        return *result;
    }

    // 2. Try to return a motion-blurred frame
    if (auto result = try_motion_blur_step(
            executor, compiled_scene, plan, task, snapshot, context, cache_state, timing_state)) {
        finalize_frame_step(*result, executor, compiled_scene, plan, task, context, cache_state);
        return *result;
    }

    // 3. Normal execution pipeline
    ExecutedFrame result;
    result.frame_number = task.frame_number;
    result.scene_hash = compiled_scene.scene_hash;

    evaluate_frame_graph_step(
        executor, compiled_scene, plan, task, snapshot, context, cache_state, timing_state);

    evaluate_and_rasterize_root_composition_step(
        result, executor, compiled_scene, plan, task, snapshot, context, cache_state, timing_state);

    finalize_frame_step(result, executor, compiled_scene, plan, task, context, cache_state);

    return result;
}

} // namespace tachyon
