#include "frame_executor_internal.h"
#include "tachyon/core/shapes/shape_path.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include "tachyon/text/content/word_timestamps.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include "tachyon/runtime/execution/property_sampling.h"
#include "tachyon/core/scene/evaluator/layer_utils.h"
#include <chrono>
#include <filesystem>
#include <cmath>

namespace tachyon {

static LayerType resolve_layer_type_internal(std::uint32_t type_id) {
    switch (type_id) {
        case 1: return LayerType::Solid;
        case 2: return LayerType::Shape;
        case 3: return LayerType::Image;
        case 4: return LayerType::Text;
        case 5: return LayerType::Precomp;
        case 6: return LayerType::Procedural;
        case 8: return LayerType::Video;
        case 11: return LayerType::Mask;
        case 12: return LayerType::NullLayer;
        default: return LayerType::NullLayer;
    }
}

static scene::EvaluatedShapePath to_evaluated_shape_path_internal(const std::optional<ShapePathSpec>& spec) {
    scene::EvaluatedShapePath result;
    if (!spec.has_value()) return result;
    result.closed = spec->closed;
    result.points.reserve(spec->points.size());
    for (const auto& p : spec->points) {
        result.points.push_back({p.position, p.in_tangent, p.out_tangent});
    }
    return result;
}

constexpr double kPi_Internal = 3.14159265358979323846;

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
    std::optional<std::uint64_t> main_frame_key,
    std::optional<double> main_frame_time) {

    (void)composition_key;
    (void)main_frame_time;
    (void)plan;
    (void)context;
    (void)snapshot;

    const auto timing_start = std::chrono::high_resolution_clock::now();
    auto record_timing = [&]() {
        if (!context.diagnostics) return;
        const auto timing_end = std::chrono::high_resolution_clock::now();
        context.diagnostics->timings.push_back(TimingSample{
            "layer",
            std::to_string(layer.node.node_id),
            std::chrono::duration<double, std::milli>(timing_end - timing_start).count()
        });
    };

    const bool is_sub_frame = main_frame_key.has_value();
    const bool layer_motion_blur = (layer.flags & 0x10) != 0;

    if (is_sub_frame && !layer_motion_blur) {
        const std::uint64_t main_layer_key = build_node_key(*main_frame_key, layer.node);
        if (auto cached_main = executor.m_cache.lookup_layer(main_layer_key)) {
            executor.m_cache.store_layer(build_node_key(frame_key, layer.node), cached_main);
            record_timing();
            return;
        }
    }

    const std::uint64_t node_key = build_node_key(frame_key, layer.node);
    if (executor.m_cache.lookup_layer(node_key)) {
        if (context.diagnostics) context.diagnostics->layer_hits++;
        record_timing();
        return;
    }

    if (context.diagnostics) {
        context.diagnostics->layer_misses++;
        context.diagnostics->layers_evaluated++;
    }

    auto state = std::make_shared<scene::EvaluatedLayerState>();
    state->identity.layer_id = std::to_string(layer.node.node_id);
    state->identity.id = ""; 
    state->identity.name = layer.name;
    state->identity.type = resolve_layer_type_internal(layer.type_id);
    state->identity.enabled = (layer.flags & 0x01U) != 0U;
    state->identity.visible = (layer.flags & 0x02U) != 0U;
    state->identity.is_adjustment_layer = (layer.flags & 0x08U) != 0U;

    state->transform.width = layer.width;
    state->transform.height = layer.height;
    state->transform.blend_mode = "normal";
    state->transform.opacity = 1.0;
    state->transform.local_transform = math::Transform2::identity();

    state->playback.local_time_seconds = frame_time_seconds;
    state->playback.in_time = layer.in_time;
    state->playback.out_time = layer.out_time;
    state->playback.transition_in = layer.transition_in;
    state->playback.transition_out = layer.transition_out;

    state->text.content = layer.text.content;
    state->text.font_id = layer.text.font_id;
    state->text.font_size = layer.text.font_size;
    state->text.fill_color = layer.text.fill_color;
    state->text.stroke_color = layer.text.stroke_color;
    state->text.stroke_width = layer.text.stroke_width;
    state->text.box = layer.text.box;
    state->text.animators = layer.text_animators;
    state->text.highlights = layer.text_highlights;

    state->vector.line_cap = layer.vector.line_cap;
    state->vector.line_join = layer.vector.line_join;
    state->vector.miter_limit = layer.vector.miter_limit;
    if (layer.vector.shape_path.has_value()) {
        state->vector.shape_path = to_evaluated_shape_path_internal(layer.vector.shape_path);
    }

    state->effects.reserve(layer.effects.size());
    for (const auto& effect : layer.effects) {
        state->effects.push_back(effect.evaluate(frame_time_seconds));
    }

    state->procedural = layer.procedural;
    state->source.precomp_id = layer.precomp_index.has_value() ? std::make_optional(std::to_string(*layer.precomp_index)) : std::nullopt;
    state->track_matte_type = layer.matte_type;
    state->track_matte_layer_index = layer.matte_layer_index.has_value()
        ? std::make_optional(static_cast<std::size_t>(*layer.matte_layer_index))
        : std::nullopt;

    if (layer.precomp_index.has_value() && *layer.precomp_index < scene.compositions.size()) {
        const auto& nested_comp = scene.compositions[*layer.precomp_index];
        const std::uint64_t nested_key = build_node_key(frame_key, nested_comp.node);
        if (auto cached_nested = executor.m_cache.lookup_composition(nested_key)) {
            state->nested_composition = std::static_pointer_cast<scene::EvaluatedCompositionState>(std::const_pointer_cast<scene::EvaluatedCompositionState>(cached_nested));
        }
    }

    auto sample_property = [&](std::size_t index, double fallback) -> double {
        if (index >= layer.property_indices.size()) return fallback;
        const auto& track = scene.property_tracks[layer.property_indices[index]];
        const std::uint64_t prop_node_key = build_node_key(frame_key, track.node);
        CacheKeyBuilder prop_builder;
        prop_builder.add_u64(prop_node_key);
        prop_builder.add_f64(frame_time_seconds);
        const std::uint64_t prop_cache_key = prop_builder.finish();
        double sampled = fallback;
        if (executor.m_cache.lookup_property(prop_cache_key, sampled)) {
            if (context.diagnostics) context.diagnostics->property_hits++;
            return sampled;
        }
        if (context.diagnostics) context.diagnostics->property_misses++;
        return static_cast<double>(runtime::sample_compiled_property_track(track, static_cast<float>(frame_time_seconds)));
    };

    state->transform.opacity = static_cast<float>(sample_property(CompiledLayer::Opacity, 1.0));
    state->transform.local_transform.position.x = static_cast<float>(sample_property(CompiledLayer::PosX, 0.0));
    state->transform.local_transform.position.y = static_cast<float>(sample_property(CompiledLayer::PosY, 0.0));
    state->transform.local_transform.scale.x = static_cast<float>(sample_property(CompiledLayer::ScaleX, 1.0));
    state->transform.local_transform.scale.y = static_cast<float>(sample_property(CompiledLayer::ScaleY, 1.0));
    state->transform.local_transform.rotation_rad = static_cast<float>(sample_property(CompiledLayer::Rotation, 0.0) * (kPi_Internal / 180.0f));
    state->transform.local_transform.anchor_point = {
        static_cast<float>(sample_property(CompiledLayer::AnchorX, 0.0)),
        static_cast<float>(sample_property(CompiledLayer::AnchorY, 0.0))
    };

    state->transform.world_matrix = math::Matrix3x3::make_translation(math::Vector2{state->transform.local_transform.position.x, state->transform.local_transform.position.y}) *
        math::Matrix3x3::make_rotation(state->transform.local_transform.rotation_rad) *
        math::Matrix3x3::make_scale(state->transform.local_transform.scale.x, state->transform.local_transform.scale.y) *
        math::Matrix3x3::make_translation(math::Vector2{-state->transform.local_transform.anchor_point.x, -state->transform.local_transform.anchor_point.y});

    const auto is_auto_fade = [](const std::string& transition_id) {
        if (transition_id == "fade" || transition_id.empty() || transition_id == "none") return true;
        if (transition_id.find("tachyon.transition.") != std::string::npos) return false;
        return true;
    };

    float transition_opacity = 1.0f;
    if (layer.transition_in.duration > 0.0 && layer.transition_in.transition_id != "none" && is_auto_fade(layer.transition_in.transition_id)) {
        double t = frame_time_seconds - layer.in_time;
        if (t >= 0.0 && t < layer.transition_in.duration) {
            transition_opacity = static_cast<float>(std::clamp(t / layer.transition_in.duration, 0.0, 1.0));
        }
    }
    if (layer.transition_out.duration > 0.0 && layer.transition_out.transition_id != "none" && is_auto_fade(layer.transition_out.transition_id)) {
        double t = layer.out_time - frame_time_seconds;
        if (t >= 0.0 && t < layer.transition_out.duration) {
            transition_opacity *= static_cast<float>(std::clamp(t / layer.transition_out.duration, 0.0, 1.0));
        }
    }
    state->transform.opacity *= transition_opacity;

    state->identity.active = state->identity.enabled && state->identity.visible && state->transform.opacity > 0.0 && 
                             frame_time_seconds >= layer.in_time && frame_time_seconds < layer.out_time;

    executor.m_cache.store_layer(node_key, std::move(state));
    record_timing();
}

} // namespace tachyon
