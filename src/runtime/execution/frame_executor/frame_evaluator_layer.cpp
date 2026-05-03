#include "frame_executor_internal.h"
#include "tachyon/core/shapes/shape_path.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include "tachyon/text/content/word_timestamps.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/math/quaternion.h"
#include "tachyon/core/scene/evaluator/layer_utils.h"
#include <chrono>
#include <filesystem>
#include <cmath>

namespace tachyon {

namespace {

scene::LayerType resolve_layer_type(std::uint32_t type_id) {
    switch (type_id) {
        case 1: return scene::LayerType::Solid;
        case 2: return scene::LayerType::Shape;
        case 3: return scene::LayerType::Image;
        case 4: return scene::LayerType::Text;
        case 5: return scene::LayerType::Precomp;
        case 6: return scene::LayerType::Procedural;
        default: return scene::LayerType::NullLayer;
    }
}

ShapePathSpec to_shape_path_spec(const std::optional<ShapePathSpec>& spec) {
    ShapePathSpec result;
    if (!spec.has_value()) return result;
    result.closed = spec->closed;
    result.points = spec->points;
    return result;
}

constexpr double kPi = 3.14159265358979323846;

} // namespace

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
        if (!context.diagnostic_tracker) {
            return;
        }
        const auto timing_end = std::chrono::high_resolution_clock::now();
        context.diagnostic_tracker->timings.push_back(TimingSample{
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
        if (context.diagnostic_tracker) context.diagnostic_tracker->layer_hits++;
        record_timing();
        return;
    }

    if (context.diagnostic_tracker) {
        context.diagnostic_tracker->layer_misses++;
        context.diagnostic_tracker->layers_evaluated++;
    }

    auto state = std::make_shared<scene::EvaluatedLayerState>();
    state->layer_index = layer.node.topo_index;
    state->id = std::to_string(layer.node.node_id);
    state->type = resolve_layer_type(layer.type_id);

    state->enabled = (layer.flags == 0U) || ((layer.flags & 0x01U) != 0U);
    state->visible = (layer.flags == 0U) || ((layer.flags & 0x02U) != 0U);
    state->is_3d = (layer.flags & 0x04U) != 0U;
    state->is_adjustment_layer = (layer.flags & 0x08U) != 0U;

    state->width = layer.width;
    state->height = layer.height;
    state->blend_mode = "normal";
    state->local_time_seconds = frame_time_seconds;
    state->child_time_seconds = frame_time_seconds;
    state->opacity = 1.0;
    state->local_transform = math::Transform2::identity();
    state->fill_color = layer.fill_color;
    state->stroke_color = layer.stroke_color;
    state->stroke_width = layer.stroke_width;
    state->line_cap = layer.line_cap;
    state->line_join = layer.line_join;
    state->miter_limit = layer.miter_limit;
    state->text_content = layer.text_content;
    state->font_id = layer.font_id;
    state->font_size = layer.font_size;
    state->text_alignment = layer.text_alignment;
    state->text_animators = layer.text_animators;
    state->text_highlights = layer.text_highlights;
    state->subtitle_path = layer.subtitle_path;
    // Sample animated subtitle outline color to static ColorSpec
    if (layer.subtitle_outline_color.value.has_value()) {
        state->subtitle_outline_color = layer.subtitle_outline_color.value;
    } else {
        state->subtitle_outline_color = std::nullopt;
    }
    state->subtitle_outline_width = layer.subtitle_outline_width;
    state->word_timestamp_path = layer.word_timestamp_path;
    state->shape_path = to_shape_path_spec(layer.shape_path);
    state->shape_spec = layer.shape_spec;
    state->effects = layer.effects;
    state->procedural = layer.procedural;
    state->precomp_id = layer.precomp_index.has_value() ? std::make_optional(std::to_string(*layer.precomp_index)) : std::nullopt;
    state->track_matte_type = layer.matte_type;
    state->track_matte_layer_index = layer.matte_layer_index.has_value()
        ? std::make_optional(static_cast<std::size_t>(*layer.matte_layer_index))
        : std::nullopt;

    if (layer.precomp_index.has_value() && *layer.precomp_index < scene.compositions.size()) {
        const auto& nested_comp = scene.compositions[*layer.precomp_index];
        const std::uint64_t nested_key = build_node_key(frame_key, nested_comp.node);
        if (auto cached_nested = executor.m_cache.lookup_composition(nested_key)) {
            state->nested_composition = std::make_shared<scene::EvaluatedCompositionState>(*cached_nested);
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
            if (context.diagnostic_tracker) context.diagnostic_tracker->property_hits++;
            return sampled;
        }
        if (context.diagnostic_tracker) context.diagnostic_tracker->property_misses++;
        return fallback;
    };

    state->opacity = static_cast<float>(sample_property(CompiledLayer::Opacity, 1.0));
    state->local_transform.position.x = static_cast<float>(sample_property(CompiledLayer::PosX, 0.0));
    state->local_transform.position.y = static_cast<float>(sample_property(CompiledLayer::PosY, 0.0));
    state->local_transform.scale.x = static_cast<float>(sample_property(CompiledLayer::ScaleX, 1.0));
    state->local_transform.scale.y = static_cast<float>(sample_property(CompiledLayer::ScaleY, 1.0));
    state->local_transform.rotation_rad = static_cast<float>(sample_property(CompiledLayer::Rotation, 0.0) * (kPi / 180.0f));
    state->mask_feather = static_cast<float>(sample_property(CompiledLayer::MaskFeather, 0.0));

    if (state->is_3d) {
        const math::Vector3 pos3{
            static_cast<float>(sample_property(CompiledLayer::PosX, 0.0)),
            static_cast<float>(sample_property(CompiledLayer::PosY, 0.0)),
            static_cast<float>(sample_property(CompiledLayer::PosZ, 0.0))
        };
        const math::Vector3 rot3{
            static_cast<float>(sample_property(CompiledLayer::RotationX, 0.0)),
            static_cast<float>(sample_property(CompiledLayer::RotationY, 0.0)),
            static_cast<float>(sample_property(CompiledLayer::RotationZ, 0.0))
        };
        const math::Vector3 scale3{
            static_cast<float>(sample_property(CompiledLayer::ScaleX, 1.0)),
            static_cast<float>(sample_property(CompiledLayer::ScaleY, 1.0)),
            static_cast<float>(sample_property(CompiledLayer::ScaleZ, 1.0))
        };
        const math::Vector3 anchor3{
            static_cast<float>(sample_property(CompiledLayer::AnchorX, static_cast<double>(state->width) * 0.5)),
            static_cast<float>(sample_property(CompiledLayer::AnchorY, static_cast<double>(state->height) * 0.5)),
            static_cast<float>(sample_property(CompiledLayer::AnchorZ, 0.0))
        };

        state->world_position3 = pos3;
        state->scale_3d = scale3;
        state->world_matrix = math::compose_trs(pos3, math::Quaternion::from_euler(rot3), scale3);

        // Previous state
        const double frame_duration = 1.0 / (scene.compositions.empty() ? 60.0 : scene.compositions[0].fps);
        const double prev_t = frame_time_seconds - frame_duration;
        auto sample_prev = [&](std::size_t index, double fallback) -> double {
            if (index >= layer.property_indices.size()) return fallback;
            return sample_keyframed_value(scene.property_tracks[layer.property_indices[index]], fallback, prev_t);
        };

        const math::Vector3 prev_pos3{
            static_cast<float>(sample_prev(CompiledLayer::PosX, 0.0)),
            static_cast<float>(sample_prev(CompiledLayer::PosY, 0.0)),
            static_cast<float>(sample_prev(CompiledLayer::PosZ, 0.0))
        };
        const math::Vector3 prev_rot3{
            static_cast<float>(sample_prev(CompiledLayer::RotationX, 0.0)),
            static_cast<float>(sample_prev(CompiledLayer::RotationY, 0.0)),
            static_cast<float>(sample_prev(CompiledLayer::RotationZ, 0.0))
        };
        const math::Vector3 prev_scale3{
            static_cast<float>(sample_prev(CompiledLayer::ScaleX, 1.0)),
            static_cast<float>(sample_prev(CompiledLayer::ScaleY, 1.0)),
            static_cast<float>(sample_prev(CompiledLayer::ScaleZ, 1.0))
        };
        state->previous_world_matrix = math::compose_trs(prev_pos3, math::Quaternion::from_euler(prev_rot3), prev_scale3);

        // Populate mesh for primitives
        if (state->type == scene::LayerType::Solid || state->type == scene::LayerType::Image || state->type == scene::LayerType::Video) {
            state->mesh_asset = scene::create_quad_mesh(static_cast<float>(state->width), static_cast<float>(state->height));
        }

        // Material properties
        state->material.metallic = static_cast<float>(sample_property(CompiledLayer::Metallic, 0.0));
        state->material.roughness = static_cast<float>(sample_property(CompiledLayer::Roughness, 0.5));
        state->material.ior = static_cast<float>(sample_property(CompiledLayer::IOR, 1.45));
        state->material.transmission = static_cast<float>(sample_property(CompiledLayer::Transmission, 0.0));
        state->material.emission = static_cast<float>(sample_property(CompiledLayer::EmissionStrength, 0.0));
    } else {
        state->world_position3 = {state->local_transform.position.x, state->local_transform.position.y, 0.0f};
        state->scale_3d = {state->local_transform.scale.x, state->local_transform.scale.y, 1.0f};
        state->world_matrix = math::compose_trs(
            state->world_position3,
            math::Quaternion::from_euler({0.0f, 0.0f, static_cast<float>(sample_property(CompiledLayer::Rotation, 0.0))}),
            state->scale_3d);
        state->previous_world_matrix = state->world_matrix;
    }

    state->active = state->enabled && state->visible && state->opacity > 0.0;

    executor.m_cache.store_layer(node_key, std::move(state));
    record_timing();
}

} // namespace tachyon
