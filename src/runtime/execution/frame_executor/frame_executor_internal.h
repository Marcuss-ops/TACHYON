#pragma once
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/runtime/cache/cache_key_builder.h"
#include "tachyon/timeline/frame_blend.h"
#include "tachyon/renderer2d/core/framebuffer.h"

namespace tachyon {

struct FrameCacheState {
    CacheKeyBuilder composition_builder;
    CacheKeyBuilder frame_builder;
    std::uint64_t composition_key{0};
    std::uint64_t frame_key{0};
    bool diagnostics_enabled{false};
};

struct FrameTimingState {
    double fps{60.0};
    double frame_time_seconds{0.0};
    std::optional<timeline::FrameBlendResult> blend_result;
};

// Internal evaluation functions
void evaluate_node(
    FrameExecutor& executor,
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
    FrameExecutor& executor,
    const CompiledScene& scene,
    const CompiledPropertyTrack& track,
    const RenderPlan& plan,
    const DataSnapshot& snapshot,
    RenderContext& context,
    std::uint64_t node_key,
    double frame_time_seconds);

void evaluate_layer(
    FrameExecutor& executor,
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
    FrameExecutor& executor,
    const CompiledScene& scene,
    const CompiledComposition& comp,
    const RenderPlan& plan,
    const DataSnapshot& snapshot,
    RenderContext& context,
    const std::uint64_t composition_key,
    std::uint64_t node_key,
    std::uint64_t frame_key,
    double frame_time_seconds,
    const FrameRenderTask& task,
    std::optional<std::uint64_t> main_frame_key = std::nullopt,
    std::optional<double> main_frame_time = std::nullopt);

// Helpers
std::uint64_t build_node_key(std::uint64_t global_key, const CompiledNode& node);

// Common render helpers
std::shared_ptr<renderer2d::SurfaceRGBA> render_frame_at_time(
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context,
    double render_time,
    std::uint64_t composition_key,
    std::uint64_t frame_key);

timeline::FrameBuffer surface_to_framebuffer(const renderer2d::SurfaceRGBA& src);

std::shared_ptr<renderer2d::Framebuffer> framebuffer_to_framebuffer(const timeline::FrameBuffer& src);

FrameCacheState build_frame_cache_state(
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    bool diagnostics_enabled);

FrameTimingState resolve_frame_timing(
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task);

ExecutedFrame run_frame_execution_pipeline(
    FrameExecutor& executor,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context,
    const FrameCacheState& cache_state,
    const FrameTimingState& timing_state);

} // namespace tachyon
