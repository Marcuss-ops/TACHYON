#include "tachyon/runtime/frame_executor.h"

#include "tachyon/renderer2d/rasterizer.h"
#include "tachyon/scene/evaluator.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace tachyon {
namespace {

renderer2d::Color background_color(const RenderPlan& plan) {
    if (plan.composition.background.has_value() && *plan.composition.background == "white") {
        return renderer2d::Color::white();
    }
    return renderer2d::Color::transparent();
}

renderer2d::Color layer_color(const scene::EvaluatedLayerState& layer, std::size_t order) {
    const std::uint8_t alpha = static_cast<std::uint8_t>(std::clamp(layer.opacity * 255.0, 0.0, 255.0));
    if (layer.type == "solid") {
        return {40, static_cast<std::uint8_t>(120 + (order * 23U) % 120U), 220, alpha};
    }
    if (layer.type == "text") {
        return {240, 240, 240, alpha};
    }
    if (layer.type == "shape") {
        return {240, 150, 40, alpha};
    }
    return {120, static_cast<std::uint8_t>(80 + (order * 31U) % 120U), static_cast<std::uint8_t>(140 + (order * 17U) % 100U), alpha};
}

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

renderer2d::RectPrimitive build_rect_for_layer(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& composition_state,
    std::size_t order) {

    const int base_width = std::max(24, static_cast<int>(composition_state.width / 4));
    const int base_height = std::max(24, static_cast<int>(composition_state.height / 6));
    const int width = std::max(8, static_cast<int>(std::lround(base_width * std::max(0.2f, layer.scale.x))));
    const int height = std::max(8, static_cast<int>(std::lround(base_height * std::max(0.2f, layer.scale.y))));
    const int center_x = static_cast<int>(composition_state.width / 2.0 + layer.position.x);
    const int center_y = static_cast<int>(composition_state.height / 2.0 + layer.position.y);
    const int y_bias = static_cast<int>(order * 12);

    return renderer2d::RectPrimitive{
        center_x - width / 2,
        center_y - height / 2 + y_bias,
        width,
        height,
        layer_color(layer, order)
    };
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

DrawList2D build_draw_list(const EvaluatedFrameState& state) {
    DrawList2D list;
    std::size_t order = 0;
    for (const auto& layer : state.composition_state.layers) {
        if (!layer.active || layer.is_camera || layer.opacity <= 0.0) {
            ++order;
            continue;
        }
        list.rects.push_back(build_rect_for_layer(layer, state.composition_state, order));
        ++order;
    }
    return list;
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

    const DrawList2D draw_list = build_draw_list(state);
    renderer2d::Framebuffer frame(
        static_cast<std::uint32_t>(state.composition_state.width),
        static_cast<std::uint32_t>(state.composition_state.height));
    frame.clear(background_color(plan));

    for (const auto& rect : draw_list.rects) {
        renderer2d::CPURasterizer::draw_rect(frame, rect);
    }

    if (task.cacheable) {
        CachedFrame cached;
        cached.entry.key = task.cache_key;
        cached.entry.note = state.composition_summary;
        cached.state_fingerprint = state.state_fingerprint;
        cached.frame = frame;
        cached.invalidates_when_changed = task.invalidates_when_changed;
        cache.store(std::move(cached));
    }

    ExecutedFrame executed;
    executed.frame_number = task.frame_number;
    executed.cache_key = task.cache_key;
    executed.cache_hit = false;
    executed.state_fingerprint = state.state_fingerprint;
    executed.draw_command_count = draw_list.rects.size();
    executed.frame = std::move(frame);
    return executed;
}

} // namespace tachyon
