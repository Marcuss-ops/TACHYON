#include "frame_executor_internal.h"
#include "tachyon/core/shapes/shape_path.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include "tachyon/text/content/word_timestamps.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include <filesystem>

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
        case 7: return scene::LayerType::UIHook;
        case 8: return scene::LayerType::Camera;
        case 9: return scene::LayerType::Light;
        case 10: return scene::LayerType::NullLayer;
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

math::Matrix4x4 build_world_matrix_3d(
    const math::Vector3& position,
    const math::Vector3& orientation_deg,
    const math::Vector3& rotation_deg,
    const math::Vector3& scale,
    const math::Vector3& anchor_point) {

    constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;
    const math::Vector3 total_rotation_deg = orientation_deg + rotation_deg;
    const math::Matrix4x4 translation = math::Matrix4x4::translation(position);
    const math::Matrix4x4 rotation = math::Matrix4x4::rotation_x(total_rotation_deg.x * kDegToRad)
        * math::Matrix4x4::rotation_y(total_rotation_deg.y * kDegToRad)
        * math::Matrix4x4::rotation_z(total_rotation_deg.z * kDegToRad);
    const math::Matrix4x4 anchor = math::Matrix4x4::translation({
        -anchor_point.x,
        -anchor_point.y,
        -anchor_point.z
    });
    const math::Matrix4x4 scale_mat = math::Matrix4x4::scaling(scale);
    return translation * rotation * anchor * scale_mat;
}

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

    const bool is_sub_frame = main_frame_key.has_value();
    const bool layer_motion_blur = (layer.flags & 0x10) != 0;

    if (is_sub_frame && !layer_motion_blur) {
        const std::uint64_t main_layer_key = build_node_key(*main_frame_key, layer.node);
        if (auto cached_main = executor.m_cache.lookup_layer(main_layer_key)) {
            executor.m_cache.store_layer(build_node_key(frame_key, layer.node), cached_main);
            return;
        }
    }

    const std::uint64_t node_key = build_node_key(frame_key, layer.node);
    if (executor.m_cache.lookup_layer(node_key)) {
        if (context.diagnostic_tracker) context.diagnostic_tracker->layer_hits++;
        return;
    }

    if (context.diagnostic_tracker) {
        context.diagnostic_tracker->layer_misses++;
        context.diagnostic_tracker->layers_evaluated++;
    }

    auto state = std::make_shared<scene::EvaluatedLayerState>();
    state->layer_index = layer.node.topo_index;
    state->id = layer.source_id.empty() ? std::to_string(layer.node.node_id) : layer.source_id;
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
    state->procedural = layer.procedural;
    state->font_size = layer.font_size;
    state->text_alignment = layer.text_alignment;
    state->extrusion_depth = layer.extrusion_depth;
    state->bevel_size = layer.bevel_size;
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
    state->animated_effects = layer.animated_effects;
    state->camera_type = layer.camera_type;
    state->zoom = layer.camera_zoom;
    state->poi = layer.camera_poi.value.value_or(math::Vector3{0.0f, 0.0f, 0.0f});
    state->camera_focus_target_id = layer.camera_focus_target_id;
    state->camera_action_aware = layer.camera_action_aware;
    state->camera_action_pan_strength = static_cast<float>(layer.camera_action_pan_strength.value.value_or(0.0));
    state->camera_snap_to_grid = layer.camera_snap_to_grid;
    state->camera_grid_size = static_cast<float>(layer.camera_grid_size.value.value_or(1.0));
    state->camera_shake_seed = layer.camera_shake_seed;
    state->camera_shake_amplitude_pos = static_cast<float>(layer.camera_shake_amplitude_pos.value.value_or(0.0));
    state->camera_shake_amplitude_rot = static_cast<float>(layer.camera_shake_amplitude_rot.value.value_or(0.0));
    state->camera_shake_frequency = static_cast<float>(layer.camera_shake_frequency.value.value_or(1.0));
    state->camera_shake_roughness = static_cast<float>(layer.camera_shake_roughness.value.value_or(0.5));
    state->ui_hook_camera_id = layer.ui_hook_camera_id;
    state->light_type = layer.light_type;
    state->light_intensity = static_cast<float>(layer.light_intensity.value.value_or(1.0));
    state->light_color = layer.light_color.value.value_or(ColorSpec{255, 255, 255, 255});
    state->falloff_type = layer.falloff_type;
    state->attenuation_near = static_cast<float>(layer.attenuation_near.value.value_or(0.0));
    state->attenuation_far = static_cast<float>(layer.attenuation_far.value.value_or(0.0));
    state->cone_angle = static_cast<float>(layer.cone_angle.value.value_or(45.0));
    state->cone_feather = static_cast<float>(layer.cone_feather.value.value_or(0.0));
    state->casts_shadows = layer.casts_shadows;
    state->shadow_darkness = static_cast<float>(layer.shadow_darkness.value.value_or(0.0));
    state->shadow_radius = static_cast<float>(layer.shadow_radius.value.value_or(0.0));
    state->precomp_id = layer.precomp_index.has_value() ? std::make_optional(std::to_string(*layer.precomp_index)) : std::nullopt;
    state->track_matte_type = layer.matte_type;
    state->track_matte_layer_index = layer.matte_layer_index.has_value()
        ? std::make_optional(static_cast<std::size_t>(*layer.matte_layer_index))
        : std::nullopt;
    state->in_time = layer.in_time;
    state->out_time = layer.out_time;
    // Note: Transitions are now handled as pixel-level transitions in composition_renderer.cpp
    // via the unified TransitionRegistry (LayerTransitionSpec::transition_id)

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
        
        CacheKeyBuilder prop_builder;
        if (track.kind == CompiledPropertyTrack::Kind::Constant) {
            prop_builder.add_u32(track.node.node_id);
            prop_builder.add_u32(0xCA57);
        } else {
            const std::uint64_t prop_node_key = build_node_key(frame_key, track.node);
            prop_builder.add_u64(prop_node_key);
            prop_builder.add_f64(frame_time_seconds);
        }
        
        const std::uint64_t prop_cache_key = prop_builder.finish();
        double sampled = fallback;
        if (executor.m_cache.lookup_property(prop_cache_key, sampled)) {
            if (context.diagnostic_tracker) context.diagnostic_tracker->property_hits++;
            return sampled;
        }
        if (context.diagnostic_tracker) context.diagnostic_tracker->property_misses++;
        return fallback;
    };

    state->opacity = static_cast<float>(sample_property(0, 1.0));
    state->local_transform.position.x = static_cast<float>(sample_property(1, 0.0));
    state->local_transform.position.y = static_cast<float>(sample_property(2, 0.0));
    state->local_transform.scale.x = static_cast<float>(sample_property(3, 1.0));
    state->local_transform.scale.y = static_cast<float>(sample_property(4, 1.0));
    state->local_transform.rotation_rad = static_cast<float>(sample_property(5, 0.0) * (kPi / 180.0f));
    state->mask_feather = static_cast<float>(sample_property(6, 0.0));
    state->local_transform.anchor_point.x = static_cast<float>(sample_property(7, 0.0));
    state->local_transform.anchor_point.y = static_cast<float>(sample_property(8, 0.0));

    // Note: Transitions are now handled as pixel-level transitions in composition_renderer.cpp
    // via the unified TransitionRegistry. LayerTransitionSpec::transition_id is used
    // to look up transitions (including base ones like "fade", "slide", "zoom").

    state->active = state->enabled && state->visible && state->opacity > 0.0
                   && frame_time_seconds >= layer.in_time && frame_time_seconds < layer.out_time;

    const bool use_3d_transform = state->is_3d || state->type == scene::LayerType::Camera || state->type == scene::LayerType::Light || state->type == scene::LayerType::UIHook;
    if (use_3d_transform) {
        const math::Vector3 pos3 = scene::sample_vector3(layer.transform3d.position_property,
            layer.transform3d.position_property.value.value_or(math::Vector3{0.0f, 0.0f, 0.0f}),
            frame_time_seconds);
        const math::Vector3 orientation3 = scene::sample_vector3(layer.transform3d.orientation_property,
            layer.transform3d.orientation_property.value.value_or(math::Vector3{0.0f, 0.0f, 0.0f}),
            frame_time_seconds);
        const math::Vector3 rot3 = scene::sample_vector3(layer.transform3d.rotation_property,
            layer.transform3d.rotation_property.value.value_or(math::Vector3{0.0f, 0.0f, 0.0f}),
            frame_time_seconds);
        const math::Vector3 scale3 = scene::sample_vector3(layer.transform3d.scale_property,
            layer.transform3d.scale_property.value.value_or(math::Vector3{1.0f, 1.0f, 1.0f}),
            frame_time_seconds);
        const math::Vector3 anchor3 = scene::sample_vector3(layer.transform3d.anchor_point_property,
            math::Vector3{static_cast<float>(layer.width) * 0.5f, static_cast<float>(layer.height) * 0.5f, 0.0f},
            frame_time_seconds);
        const math::Vector3 prev_pos3 = scene::sample_vector3(layer.transform3d.position_property,
            layer.transform3d.position_property.value.value_or(math::Vector3{0.0f, 0.0f, 0.0f}),
            main_frame_time.value_or(frame_time_seconds));
        const math::Vector3 prev_orientation3 = scene::sample_vector3(layer.transform3d.orientation_property,
            layer.transform3d.orientation_property.value.value_or(math::Vector3{0.0f, 0.0f, 0.0f}),
            main_frame_time.value_or(frame_time_seconds));
        const math::Vector3 prev_rot3 = scene::sample_vector3(layer.transform3d.rotation_property,
            layer.transform3d.rotation_property.value.value_or(math::Vector3{0.0f, 0.0f, 0.0f}),
            main_frame_time.value_or(frame_time_seconds));
        const math::Vector3 prev_scale3 = scene::sample_vector3(layer.transform3d.scale_property,
            layer.transform3d.scale_property.value.value_or(math::Vector3{1.0f, 1.0f, 1.0f}),
            main_frame_time.value_or(frame_time_seconds));
        const math::Vector3 prev_anchor3 = scene::sample_vector3(layer.transform3d.anchor_point_property,
            math::Vector3{static_cast<float>(layer.width) * 0.5f, static_cast<float>(layer.height) * 0.5f, 0.0f},
            main_frame_time.value_or(frame_time_seconds));

        state->world_matrix = build_world_matrix_3d(pos3, orientation3, rot3, scale3, anchor3);
        state->world_position3 = state->world_matrix.transform_point({0.0f, 0.0f, 0.0f});
        state->world_normal = state->world_matrix.transform_vector({0.0f, 0.0f, 1.0f}).normalized();
        state->scale_3d = scale3;
        state->previous_world_matrix = build_world_matrix_3d(prev_pos3, prev_orientation3, prev_rot3, prev_scale3, prev_anchor3);
    }

    executor.m_cache.store_layer(node_key, std::move(state));
}

} // namespace tachyon
