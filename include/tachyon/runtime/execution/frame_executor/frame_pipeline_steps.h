#pragma once
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/cache/cache_key_builder.h"
#include "tachyon/timeline/frame_blend.h"
#include <optional>

namespace tachyon {
 
/**
 * @brief State for frame caching and identity.
 */
struct FrameCacheState {
    CacheKeyBuilder composition_builder;
    CacheKeyBuilder frame_builder;
    std::uint64_t composition_key{0};
    std::uint64_t frame_key{0};
    bool diagnostics_enabled{false};
};

/**
 * @brief State for frame timing and motion blur.
 */
struct FrameTimingState {
    double fps{60.0};
    double frame_time_seconds{0.0};
    std::optional<timeline::FrameBlendResult> blend_result;
};

/**
 * @brief Logic for attempting to return a blended frame (e.g. for time-stretching).
 */
std::optional<ExecutedFrame> try_frame_blend_step(
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context,
    const FrameCacheState& cache_state,
    const FrameTimingState& timing_state
);

/**
 * @brief Logic for attempting to return a motion-blurred frame.
 */
std::optional<ExecutedFrame> try_motion_blur_step(
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context,
    const FrameCacheState& cache_state,
    const FrameTimingState& timing_state
);

/**
 * @brief Evaluates all nodes in the scene graph for the current frame.
 */
void evaluate_frame_graph_step(
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context,
    const FrameCacheState& cache_state,
    const FrameTimingState& timing_state
);

/**
 * @brief Evaluates the root composition and rasterizes it into the result frame.
 */
void evaluate_and_rasterize_root_composition_step(
    ExecutedFrame& result,
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context,
    const FrameCacheState& cache_state,
    const FrameTimingState& timing_state
);

/**
 * @brief Finalizes frame result and cleans up per-frame diagnostics.
 */
void finalize_frame_step(
    ExecutedFrame& result,
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext& context,
    const FrameCacheState& cache_state
);

} // namespace tachyon
