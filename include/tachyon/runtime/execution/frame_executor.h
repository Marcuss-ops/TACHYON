#pragma once

#include "tachyon/renderer2d/draw_command.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include "tachyon/runtime/execution/render_plan.h"
#include "tachyon/runtime/core/render_graph.h"
#include "tachyon/core/scene/evaluated_state.h"
#include "tachyon/core/spec/scene_spec.h"
#include "tachyon/runtime/core/compiled_scene.h"

#include <cstddef>
#include <string>

namespace tachyon {

struct EvaluatedFrameState {
    FrameRenderTask task;
    scene::EvaluatedCompositionState composition_state;
    std::uint64_t scene_hash{0};
    std::string composition_summary;
};

struct ExecutedFrame {
    std::int64_t frame_number{0};
    FrameCacheKey cache_key;
    bool cache_hit{false};
    std::uint64_t scene_hash{0};
    std::size_t draw_command_count{0};
    renderer2d::Framebuffer frame{1, 1};
};

EvaluatedFrameState evaluate_frame_state(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task);
EvaluatedFrameState evaluate_frame_state(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    std::uint64_t scene_hash);
renderer2d::DrawList2D build_draw_list(const EvaluatedFrameState& state);
ExecutedFrame execute_frame_task(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    FrameCache& cache,
    RenderContext& context);

} // namespace tachyon
