#include "tachyon/runtime/frame_executor.h"

#include "tachyon/renderer2d/evaluated_composition_renderer.h"
#include "tachyon/renderer2d/draw_list_builder.h"
#include "tachyon/renderer2d/draw_list_rasterizer.h"
#include "tachyon/scene/evaluator.h"

#include <sstream>
#include <stdexcept>

namespace tachyon {
namespace {

void append_part(std::ostringstream& stream, const char* key, const std::string& value) {
    stream << key << '=' << value << ';';
}

void append_part(std::ostringstream& stream, const char* key, std::int64_t value) {
    stream << key << '=' << value << ';';
}

void append_part(std::ostringstream& stream, const char* key, double value) {
    stream << key << '=' << value << ';';
}

void append_part(std::ostringstream& stream, const char* key, bool value) {
    stream << key << '=' << (value ? 1 : 0) << ';';
}

void append_optional(std::ostringstream& stream, const char* key, const std::optional<std::string>& value) {
    if (value.has_value()) {
        append_part(stream, key, *value);
    }
}

void append_optional(std::ostringstream& stream, const char* key, const std::optional<double>& value) {
    if (value.has_value()) {
        append_part(stream, key, *value);
    }
}

std::string build_scene_signature(const SceneSpec& scene) {
    std::ostringstream stream;
    append_part(stream, "spec_version", scene.spec_version);
    append_part(stream, "project.id", scene.project.id);
    append_part(stream, "project.name", scene.project.name);
    append_part(stream, "asset_count", static_cast<std::int64_t>(scene.assets.size()));
    for (const auto& asset : scene.assets) {
        append_part(stream, "asset.id", asset.id);
        append_part(stream, "asset.type", asset.type);
        append_part(stream, "asset.source", asset.source);
    }

    append_part(stream, "composition_count", static_cast<std::int64_t>(scene.compositions.size()));
    for (const auto& composition : scene.compositions) {
        append_part(stream, "composition.id", composition.id);
        append_part(stream, "composition.name", composition.name);
        append_part(stream, "composition.width", composition.width);
        append_part(stream, "composition.height", composition.height);
        append_part(stream, "composition.duration", composition.duration);
        append_part(stream, "composition.frame_rate.numerator", composition.frame_rate.numerator);
        append_part(stream, "composition.frame_rate.denominator", composition.frame_rate.denominator);
        append_optional(stream, "composition.background", composition.background);
        append_part(stream, "composition.layer_count", static_cast<std::int64_t>(composition.layers.size()));

        for (const auto& layer : composition.layers) {
            append_part(stream, "layer.id", layer.id);
            append_part(stream, "layer.type", layer.type);
            append_part(stream, "layer.name", layer.name);
            append_part(stream, "layer.blend_mode", layer.blend_mode);
            append_part(stream, "layer.enabled", layer.enabled);
            append_part(stream, "layer.start_time", layer.start_time);
            append_part(stream, "layer.in_point", layer.in_point);
            append_part(stream, "layer.out_point", layer.out_point);
            append_part(stream, "layer.opacity", layer.opacity);
            append_optional(stream, "layer.parent", layer.parent);
            append_optional(stream, "layer.transform.position_x", layer.transform.position_x);
            append_optional(stream, "layer.transform.position_y", layer.transform.position_y);
            append_optional(stream, "layer.transform.rotation", layer.transform.rotation);
            append_optional(stream, "layer.transform.scale_x", layer.transform.scale_x);
            append_optional(stream, "layer.transform.scale_y", layer.transform.scale_y);

            append_part(stream, "layer.opacity_keyframes", static_cast<std::int64_t>(layer.opacity_property.keyframes.size()));
            append_optional(stream, "layer.opacity_value", layer.opacity_property.value);
            append_part(stream, "layer.time_remap_keyframes", static_cast<std::int64_t>(layer.time_remap_property.keyframes.size()));
            append_optional(stream, "layer.time_remap_value", layer.time_remap_property.value);
            append_part(stream, "layer.transform.position_keyframes", static_cast<std::int64_t>(layer.transform.position_property.keyframes.size()));
            if (layer.transform.position_property.value.has_value()) {
                append_part(stream, "layer.transform.position_value_x", layer.transform.position_property.value->x);
                append_part(stream, "layer.transform.position_value_y", layer.transform.position_property.value->y);
            }
            append_part(stream, "layer.transform.rotation_keyframes", static_cast<std::int64_t>(layer.transform.rotation_property.keyframes.size()));
            append_optional(stream, "layer.transform.rotation_value", layer.transform.rotation_property.value);
            append_part(stream, "layer.transform.scale_keyframes", static_cast<std::int64_t>(layer.transform.scale_property.keyframes.size()));
            if (layer.transform.scale_property.value.has_value()) {
                append_part(stream, "layer.transform.scale_value_x", layer.transform.scale_property.value->x);
                append_part(stream, "layer.transform.scale_value_y", layer.transform.scale_property.value->y);
            }
        }
    }

    return stream.str();
}

std::string build_fingerprint(const scene::EvaluatedCompositionState& state) {
    std::ostringstream stream;
    stream << state.composition_id << ';' << state.frame_number << ';' << state.width << 'x' << state.height << ';';
    for (const auto& layer : state.layers) {
        stream << layer.id << ':' << layer.active << ':' << layer.opacity << ':'
               << layer.local_transform.position.x << ':' << layer.local_transform.position.y << ':'
               << layer.local_transform.scale.x << ':' << layer.local_transform.scale.y << ';';
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
    return evaluate_frame_state(scene, plan, task, build_scene_signature(scene));
}

EvaluatedFrameState evaluate_frame_state(
    const SceneSpec& scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const std::string& scene_signature) {

    const auto evaluated = scene::evaluate_scene_composition_state(scene, plan.composition.id, task.frame_number);
    if (!evaluated.has_value()) {
        throw std::runtime_error("failed to evaluate composition state for frame");
    }

    EvaluatedFrameState state;
    state.task = task;
    state.composition_state = *evaluated;
    state.scene_signature = scene_signature;
    state.composition_summary = build_summary(state.composition_state);
    return state;
}

renderer2d::DrawList2D build_draw_list(const EvaluatedFrameState& state) {
    return renderer2d::DrawListBuilder::build(state.composition_state);
}

ExecutedFrame execute_frame_task(
    const SceneSpec& scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    FrameCache& cache,
    RenderContext& context) {
    (void)context;
    const std::string scene_signature = build_scene_signature(scene);

    if (const CachedFrame* cached = cache.lookup(task.cache_key, scene_signature)) {
        ExecutedFrame frame;
        frame.frame_number = task.frame_number;
        frame.cache_key = task.cache_key;
        frame.cache_hit = true;
        frame.scene_signature = scene_signature;
        frame.draw_command_count = 0;
        frame.frame = cached->frame;
        return frame;
    }

    const EvaluatedFrameState state = evaluate_frame_state(scene, plan, task, scene_signature);
    const renderer2d::DrawList2D draw_list = build_draw_list(state);
    const RasterizedFrame2D rasterized = tachyon::render_evaluated_composition_2d(state.composition_state, plan, task, context.renderer2d);

    renderer2d::Framebuffer frame(
        static_cast<std::uint32_t>(state.composition_state.width),
        static_cast<std::uint32_t>(state.composition_state.height));
    if (rasterized.surface.has_value()) {
        frame = *rasterized.surface;
    }

    if (task.cacheable) {
        cache.store(CachedFrame{
            FrameCacheEntry{task.cache_key, state.composition_summary},
            state.scene_signature,
            frame,
            task.invalidates_when_changed
        });
    }

    ExecutedFrame executed;
    executed.frame_number = task.frame_number;
    executed.cache_key = task.cache_key;
    executed.cache_hit = false;
    executed.scene_signature = state.scene_signature;
    executed.draw_command_count = draw_list.commands.size();
    executed.frame = std::move(frame);
    return executed;
}

} // namespace tachyon
