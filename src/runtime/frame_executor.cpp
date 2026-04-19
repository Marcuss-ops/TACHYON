#include "tachyon/runtime/frame_executor.h"

#include "tachyon/renderer2d/draw_list_builder.h"
#include "tachyon/renderer2d/draw_list_rasterizer.h"
#include "tachyon/scene/evaluator.h"

#include <sstream>
#include <stdexcept>

namespace tachyon {
namespace {

std::string build_fingerprint(const scene::EvaluatedCompositionState& state) {
    std::ostringstream stream;
    stream << state.composition_id << ';' << state.frame_number << ';' << state.width << 'x' << state.height << ';';
    for (const auto& layer : state.layers) {
        stream << layer.id << ':' << layer.active << ':' << layer.opacity << ':'
               << layer.position.x << ':' << layer.position.y << ':'
               << layer.rotation_degrees << ':' << layer.scale.x << ':' << layer.scale.y << ';';
    }
    return stream.str();
}

std::string build_summary(const scene::EvaluatedCompositionState& state) {
    std::ostringstream stream;
    stream << state.composition_name << ' ' << state.width << 'x' << state.height << " frame=" << state.frame_number
           << " layers=" << state.layers.size();
    return stream.str();
}

} // namespace

EvaluatedFrameState evaluate_frame_state(const SceneSpec& scene, const RenderPlan& plan, const FrameRenderTask& task) {
    const auto evaluated = scene::evaluate_scene_composition_state(scene, plan.composition.id, task.frame_number);
    if (!evaluated.has_value()) {
        throw std::runtime_error("failed to evaluate composition state for frame");
    }

    EvaluatedFrameState state;
    state.task = task;
    state.composition_state = *evaluated;
    state.state_fingerprint = build_fingerprint(state.composition_state);
    state.composition_summary = build_summary(state.composition_state);
    return state;
}

renderer2d::DrawList2D build_draw_list(const EvaluatedFrameState& state) {
    return renderer2d::DrawListBuilder::build(state.composition_state);
}

ExecutedFrame execute_frame_task(const SceneSpec& scene, const RenderPlan& plan, const FrameRenderTask& task, FrameCache& cache) {
    const EvaluatedFrameState state = evaluate_frame_state(scene, plan, task);

    if (const CachedFrame* cached = cache.lookup(task.cache_key, state.state_fingerprint)) {
        ExecutedFrame frame;
        frame.frame_number = task.frame_number;
        frame.cache_key = task.cache_key;
        frame.cache_hit = true;
        frame.state_fingerprint = state.state_fingerprint;
        frame.draw_command_count = 0;
        frame.frame = cached->frame;
        return frame;
    }

    const renderer2d::DrawList2D draw_list = build_draw_list(state);
    const RasterizedFrame2D rasterized = render_draw_list_2d(plan, task, draw_list);

    renderer2d::Framebuffer frame(
        static_cast<std::uint32_t>(state.composition_state.width),
        static_cast<std::uint32_t>(state.composition_state.height));
    if (rasterized.surface.has_value()) {
        frame = *rasterized.surface;
    }

    if (task.cacheable) {
        cache.store(CachedFrame{
            FrameCacheEntry{task.cache_key, state.composition_summary},
            state.state_fingerprint,
            frame,
            task.invalidates_when_changed
        });
    }

    ExecutedFrame executed;
    executed.frame_number = task.frame_number;
    executed.cache_key = task.cache_key;
    executed.cache_hit = false;
    executed.state_fingerprint = state.state_fingerprint;
    executed.draw_command_count = draw_list.commands.size();
    executed.frame = std::move(frame);
    return executed;
}

} // namespace tachyon
