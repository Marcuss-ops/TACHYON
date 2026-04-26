#include "tachyon/core/scene/evaluator/layer_evaluator.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/core/scene/evaluator/layer_utils.h"
#include "tachyon/core/scene/evaluator/templates.h"
#include "tachyon/core/scene/math/evaluator_math.h"
#include "tachyon/core/scene/evaluator/hashing.h"
#include "tachyon/core/scene/evaluator/camera2d_evaluator.h"
#include "tachyon/core/shapes/shape_path.h"

#include <algorithm>
#include <cmath>
#include <optional>

namespace tachyon::scene {
EvaluatedLayerState make_layer_state(
    EvaluationContext& context,
    const LayerSpec& layer,
    std::size_t layer_index,
    double time_offset,
    const EvaluationVariables& vars) {

    EvaluatedLayerState evaluated;
    evaluated.layer_id = layer.id;
    evaluated.id = layer.id;
    evaluated.name = layer.name;
    evaluated.layer_index = layer_index;
    evaluated.type = map_layer_type(layer.type);
    evaluated.enabled = layer.enabled;
    evaluated.visible = layer.visible;
    evaluated.is_3d = layer.is_3d;
    evaluated.is_adjustment_layer = layer.is_adjustment_layer;
    evaluated.motion_blur = layer.motion_blur;
    evaluated.track_matte_type = layer.track_matte_type;
    evaluated.precomp_id = layer.precomp_id;
    
    evaluated.width = layer.width;
    evaluated.height = layer.height;
    evaluated.blend_mode = layer.blend_mode;

    const double t = context.composition_time_seconds + time_offset;
    double local_t = t - layer.start_time;

    // Apply loop and hold_last_frame behavior
    const double layer_duration = layer.out_point - layer.in_point;
    if (layer.loop && layer_duration > 0.0) {
        local_t = std::fmod(local_t, layer_duration);
    }
    if (layer.hold_last_frame && local_t > layer_duration) {
        local_t = layer_duration;
    }

    evaluated.local_time_seconds = local_t;
    evaluated.child_time_seconds = sample_scalar(
        layer.time_remap_property,
        local_t,
        local_t,
        context.audio_analyzer,
        make_property_expression_seed(context.scene, context.composition, layer, "time_remap"),
        vars.numeric,
        vars.tables,
        static_cast<std::uint32_t>(layer_index));
    // Apply time remap if property is not empty
    if (!layer.time_remap_property.empty()) {
        local_t = evaluated.child_time_seconds;
    }

    evaluated.active = layer.enabled && (t >= layer.in_point && t < layer.out_point);
    const double frame_duration = 1.0 / context.composition.frame_rate.value();
    double prev_local_t = (t - frame_duration) - layer.start_time;

    // Apply loop and hold_last_frame to prev_local_t as well
    if (layer.loop && layer_duration > 0.0) {
        prev_local_t = std::fmod(prev_local_t, layer_duration);
    }
    if (layer.hold_last_frame && prev_local_t > layer_duration) {
        prev_local_t = layer_duration;
    }

    // Sample Properties
    evaluated.mask_feather = static_cast<float>(sample_scalar(
        layer.mask_feather,
        0.0,
        local_t,
        context.audio_analyzer,
        make_property_expression_seed(context.scene, context.composition, layer, "mask_feather"),
        vars.numeric,
        vars.tables,
        static_cast<std::uint32_t>(layer_index)));
    evaluated.opacity = static_cast<float>(sample_scalar(
        layer.opacity_property,
        layer.opacity,
        local_t,
        context.audio_analyzer,
        make_property_expression_seed(context.scene, context.composition, layer, "opacity"),
        vars.numeric,
        vars.tables,
        static_cast<std::uint32_t>(layer_index)));

    const bool use_3d_transform = layer.is_3d || evaluated.type == LayerType::Camera;
    if (use_3d_transform) {
        const math::Vector3 position_fallback = layer.transform3d.position_property.value.value_or(math::Vector3{0.0f, 0.0f, 0.0f});
        const math::Vector3 rotation_fallback = layer.transform3d.rotation_property.value.value_or(math::Vector3{0.0f, 0.0f, 0.0f});
        const math::Vector3 scale_fallback_3d = layer.transform3d.scale_property.value.value_or(math::Vector3{1.0f, 1.0f, 1.0f});
        const math::Vector3 pos3 = sample_vector3(layer.transform3d.position_property, position_fallback, local_t, context.audio_analyzer);
        const math::Vector3 rot3 = sample_vector3(layer.transform3d.rotation_property, rotation_fallback, local_t, context.audio_analyzer);
        const math::Vector3 scale3 = sample_vector3(layer.transform3d.scale_property, scale_fallback_3d, local_t, context.audio_analyzer);

        // Sample anchor point for 3D
        const math::Vector3 anchor3 = sample_vector3(layer.transform3d.anchor_point_property,
            math::Vector3{static_cast<float>(layer.width) * 0.5f, static_cast<float>(layer.height) * 0.5f, 0.0f},
            local_t, context.audio_analyzer);


        evaluated.world_position3 = pos3;
        evaluated.scale_3d = scale3;
        evaluated.local_transform.position = {pos3.x, pos3.y};
        evaluated.local_transform.rotation_rad = 0.0f;
        evaluated.local_transform.scale = {scale3.x, scale3.y};
        evaluated.world_matrix = math::compose_trs(
            pos3,
            math::Quaternion::from_euler(rot3),
            scale3);

        const math::Vector3 prev_pos3 = sample_vector3(layer.transform3d.position_property, position_fallback, prev_local_t, context.audio_analyzer);
        const math::Vector3 prev_rot3 = sample_vector3(layer.transform3d.rotation_property, rotation_fallback, prev_local_t, context.audio_analyzer);
        const math::Vector3 prev_scale3 = sample_vector3(layer.transform3d.scale_property, scale_fallback_3d, prev_local_t, context.audio_analyzer);
        evaluated.previous_world_matrix = math::compose_trs(
            prev_pos3,
            math::Quaternion::from_euler(prev_rot3),
            prev_scale3);
    } else {
        const auto position_fallback = math::Vector2{
            static_cast<float>(layer.transform.position_x.value_or(0.0)),
            static_cast<float>(layer.transform.position_y.value_or(0.0))
        };
        const auto scale_fallback = math::Vector2{
            static_cast<float>(layer.transform.scale_x.value_or(1.0)),
            static_cast<float>(layer.transform.scale_y.value_or(1.0))
        };

        const math::Vector2 pos2 = sample_vector2(
            layer.transform.position_property,
            position_fallback,
            local_t,
            context.audio_analyzer);
        const double rot_deg = sample_scalar(layer.transform.rotation_property, layer.transform.rotation.value_or(0.0), local_t, context.audio_analyzer);
        const math::Vector2 scale2 = sample_vector2(
            layer.transform.scale_property,
            scale_fallback,
            local_t,
            context.audio_analyzer);

        // Motion Path support
        if (layer.transform.motion_path_enabled) {
            // Determine which shape path to use
            std::optional<shapes::ShapePathSpec> motion_path;

            if (layer.transform.motion_path_shape.has_value()) {
                // Use inline motion path shape
                motion_path = layer.transform.motion_path_shape;
            } else if (layer.transform.motion_path_layer_id.has_value()) {
                // TODO: Look up shape from referenced layer
                // This requires access to composition layers
            }

            if (motion_path.has_value() && !motion_path->empty()) {
                // Sample progress along path (0.0 to 1.0)
                double path_progress = sample_scalar(
                    layer.transform.motion_path_offset_property,
                    0.0,
                    local_t,
                    context.audio_analyzer);
                path_progress = std::clamp(path_progress, 0.0, 1.0);

                // Sample position from path
                shapes::Point2D path_pos = motion_path->sample_point(path_progress);
                evaluated.local_transform.position = {path_pos.x, path_pos.y};

                // Orient to path if enabled
                if (layer.transform.orient_to_path) {
                    double tangent_angle = motion_path->sample_tangent_angle(path_progress);
                    // Convert to degrees and add to existing rotation
                    evaluated.local_transform.rotation_rad = static_cast<float>(tangent_angle);
                }
            }
        }

        // Sample anchor point for 2D
        const math::Vector2 anchor2 = sample_vector2(
            layer.transform.anchor_point,
            math::Vector2{static_cast<float>(layer.width) * 0.5f, static_cast<float>(layer.height) * 0.5f},
            local_t,
            context.audio_analyzer);
        (void)anchor2;


        evaluated.local_transform.position = pos2;
        evaluated.local_transform.rotation_rad = static_cast<float>(degrees_to_radians(rot_deg));
        evaluated.local_transform.scale = scale2;
        evaluated.world_position3 = {pos2.x, pos2.y, 0.0f};
        evaluated.scale_3d = {scale2.x, scale2.y, 1.0f};
        evaluated.world_matrix = math::compose_trs(
            evaluated.world_position3,
            math::Quaternion::from_euler({0.0f, 0.0f, static_cast<float>(rot_deg)}),
            evaluated.scale_3d);

        const math::Vector2 prev_pos2 = sample_vector2(
            layer.transform.position_property,
            position_fallback,
            prev_local_t,
            context.audio_analyzer);
        const double prev_rot_deg = sample_scalar(layer.transform.rotation_property, layer.transform.rotation.value_or(0.0), prev_local_t, context.audio_analyzer);
        const math::Vector2 prev_scale2 = sample_vector2(
            layer.transform.scale_property,
            scale_fallback,
            prev_local_t,
            context.audio_analyzer);

        // Sample previous anchor point
        const math::Vector2 prev_anchor2 = sample_vector2(
            layer.transform.anchor_point,
            math::Vector2{static_cast<float>(layer.width) * 0.5f, static_cast<float>(layer.height) * 0.5f},
            prev_local_t,
            context.audio_analyzer);
        (void)prev_anchor2;

        evaluated.previous_world_matrix = math::compose_trs(
            {prev_pos2.x, prev_pos2.y, 0.0f},
            math::Quaternion::from_euler({0.0f, 0.0f, static_cast<float>(prev_rot_deg)}),
            {prev_scale2.x, prev_scale2.y, 1.0f});

        // Apply 2D camera transform with parallax
        if (layer.has_parallax && !context.composition.cameras_2d.empty()) {
            std::string camera_id = layer.camera2d_id.value_or(context.composition.active_camera2d_id.value_or(""));
            if (!camera_id.empty()) {
                auto cam_it = std::find_if(context.composition.cameras_2d.begin(), context.composition.cameras_2d.end(),
                    [&](const Camera2DSpec& c) { return c.id == camera_id; });
                if (cam_it != context.composition.cameras_2d.end()) {
                    EvaluatedCamera2D camera = evaluate_camera2d(*cam_it, local_t);
                    math::Vector2 transformed_pos = apply_camera2d_transform(camera, layer, layer.parallax_factor, pos2);
                    evaluated.world_matrix = math::compose_trs(
                        {transformed_pos.x, transformed_pos.y, 0.0f},
                        math::Quaternion::from_euler({0.0f, 0.0f, static_cast<float>(rot_deg)}),
                        {scale2.x, scale2.y, 1.0f});
                    evaluated.local_transform.position = transformed_pos;
                }
            }
        }
    }

    // Camera specific
    if (evaluated.type == LayerType::Camera) {
        evaluated.camera_type = layer.camera_type;
        evaluated.zoom = static_cast<float>(sample_scalar(layer.camera_zoom, 877.0, local_t, context.audio_analyzer));
        evaluated.poi = sample_vector3(layer.camera_poi, {0,0,0}, local_t, context.audio_analyzer);
        
        // NEW: Camera Shake
        evaluated.camera_shake_seed = layer.camera_shake_seed;
        evaluated.camera_shake_amplitude_pos = static_cast<float>(sample_scalar(layer.camera_shake_amplitude_pos, 0.0, local_t, context.audio_analyzer));
        evaluated.camera_shake_amplitude_rot = static_cast<float>(sample_scalar(layer.camera_shake_amplitude_rot, 0.0, local_t, context.audio_analyzer));
        evaluated.camera_shake_frequency = static_cast<float>(sample_scalar(layer.camera_shake_frequency, 1.0, local_t, context.audio_analyzer));
        evaluated.camera_shake_roughness = static_cast<float>(sample_scalar(layer.camera_shake_roughness, 0.5, local_t, context.audio_analyzer));
    }

    // Text specific
    evaluated.text_content = resolve_template(layer.text_content, vars.strings, vars.numeric);
    evaluated.font_id = layer.font_id;
    evaluated.font_size = static_cast<float>(sample_scalar(layer.font_size, 24.0, local_t, context.audio_analyzer));
    evaluated.fill_color = sample_color(layer.fill_color, {255,255,255,255}, local_t);
    evaluated.stroke_color = sample_color(layer.stroke_color, {0,0,0,255}, local_t);
    evaluated.stroke_width = static_cast<float>(sample_scalar(layer.stroke_width_property, layer.stroke_width, local_t, context.audio_analyzer));
    evaluated.extrusion_depth = static_cast<float>(layer.extrusion_depth);
    evaluated.bevel_size = static_cast<float>(layer.bevel_size);
    
    // Copy procedural spec if present
    if (layer.procedural.has_value()) {
        evaluated.procedural = layer.procedural;
    }

    evaluated.effects = layer.effects;
    evaluated.animated_effects = layer.animated_effects;

    return evaluated;
}

} // namespace tachyon::scene
