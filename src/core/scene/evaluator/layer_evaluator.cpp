#include "tachyon/core/scene/evaluator/layer_evaluator.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/core/scene/evaluator/layer_utils.h"
#include "tachyon/core/scene/evaluator/templates.h"
#include "tachyon/core/scene/math/evaluator_math.h"
#include "tachyon/core/scene/evaluator/hashing.h"
#include "tachyon/core/scene/evaluator/camera2d_evaluator.h"

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

        const math::Vector2 pos2 = sample_vector2(layer.transform.position_property, position_fallback, local_t, context.audio_analyzer);
        const double rot_deg = sample_scalar(layer.transform.rotation_property, layer.transform.rotation.value_or(0.0), local_t, context.audio_analyzer);
        const math::Vector2 scale2 = sample_vector2(layer.transform.scale_property, scale_fallback, local_t, context.audio_analyzer);

        // Sample anchor point for 2D
        const math::Vector2 anchor2 = sample_vector2(layer.transform.anchor_point,
            math::Vector2{static_cast<float>(layer.width) * 0.5f, static_cast<float>(layer.height) * 0.5f},
            local_t, context.audio_analyzer);


        evaluated.local_transform.position = pos2;
        evaluated.local_transform.rotation_rad = static_cast<float>(degrees_to_radians(rot_deg));
        evaluated.local_transform.scale = scale2;
        evaluated.world_position3 = {pos2.x, pos2.y, 0.0f};
        evaluated.scale_3d = {scale2.x, scale2.y, 1.0f};
        evaluated.world_matrix = math::compose_trs(
            evaluated.world_position3,
            math::Quaternion::from_euler({0.0f, 0.0f, static_cast<float>(rot_deg)}),
            evaluated.scale_3d);

        const math::Vector2 prev_pos2 = sample_vector2(layer.transform.position_property, position_fallback, prev_local_t, context.audio_analyzer);
        const double prev_rot_deg = sample_scalar(layer.transform.rotation_property, layer.transform.rotation.value_or(0.0), prev_local_t, context.audio_analyzer);
        const math::Vector2 prev_scale2 = sample_vector2(layer.transform.scale_property, scale_fallback, prev_local_t, context.audio_analyzer);

        // Sample previous anchor point
        const math::Vector2 prev_anchor2 = sample_vector2(layer.transform.anchor_point,
            math::Vector2{static_cast<float>(layer.width) * 0.5f, static_cast<float>(layer.height) * 0.5f},
            prev_local_t, context.audio_analyzer);

        evaluated.previous_world_matrix = math::compose_trs(
            {prev_pos2.x, prev_pos2.y, 0.0f},
            math::Quaternion::from_euler({0.0f, 0.0f, static_cast<float>(prev_rot_deg)}),
            {prev_scale2.x, prev_scale2.y, 1.0f});

        // Apply 2D camera transform with parallax
        if (layer.has_parallax && !context.composition.cameras_2d.empty()) {
            std::string camera_id = layer.camera2d_id.value_or(context.composition.active_camera2d_id.value_or(""));
            if (!camera_id.empty()) {
                auto it = context.camera2d_indices.find(camera_id);
                if (it != context.camera2d_indices.end()) {
                    const Camera2DSpec& camera_spec = context.composition.cameras_2d[it->second];
                    EvaluatedCamera2D camera = evaluate_camera2d(camera_spec, local_t);
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

        // NEW: Depth of Field
        evaluated.camera_aperture = sample_scalar(layer.camera_aperture, 2.8, local_t, context.audio_analyzer);
        evaluated.camera_focus_distance = sample_scalar(layer.camera_focus_distance, 10.0, local_t, context.audio_analyzer);
    }

    // Text specific
    evaluated.text_content = resolve_template(layer.text_content, vars.strings, vars.numeric);
    evaluated.font_id = layer.font_id;
    evaluated.font_size = static_cast<float>(sample_scalar(layer.font_size, 24.0, local_t, context.audio_analyzer));
    evaluated.fill_color = sample_color(layer.fill_color, {255,255,255,255}, local_t);
    evaluated.stroke_color = sample_color(layer.stroke_color, {0,0,0,255}, local_t);
    evaluated.stroke_width = static_cast<float>(sample_scalar(layer.stroke_width_property, layer.stroke_width, local_t, context.audio_analyzer));

    evaluated.effects = layer.effects;
    
    // NEW: Populate 3D mesh for primitives if is_3d is true
    if (evaluated.is_3d) {
        if (evaluated.type == LayerType::Solid || evaluated.type == LayerType::Image || evaluated.type == LayerType::Video) {
            evaluated.mesh_asset = create_quad_mesh(static_cast<float>(evaluated.width), static_cast<float>(evaluated.height));
        }
        
        // Populate material state
        evaluated.material.metallic = static_cast<float>(sample_scalar(layer.metallic, 0.0, local_t, context.audio_analyzer));
        evaluated.material.roughness = static_cast<float>(sample_scalar(layer.roughness, 0.5, local_t, context.audio_analyzer));
        evaluated.material.transmission = static_cast<float>(sample_scalar(layer.transmission, 0.0, local_t, context.audio_analyzer));
        evaluated.material.ior = static_cast<float>(sample_scalar(layer.ior, 1.45, local_t, context.audio_analyzer));
        evaluated.material.emission = static_cast<float>(sample_scalar(layer.emission_strength, 0.0, local_t, context.audio_analyzer));
    }

    // Procedural specific
    if (layer.procedural.has_value()) {
        const ProceduralSpec& spec = *layer.procedural;
        ProceduralSpec evaluated_proc = spec;
        
        const auto eval_scalar = [&](const AnimatedScalarSpec& prop, double fallback, const char* name) {
            return sample_scalar(prop, fallback, local_t, context.audio_analyzer,
                make_property_expression_seed(context.scene, context.composition, layer, name),
                vars.numeric, vars.tables, static_cast<std::uint32_t>(layer_index));
        };

        const auto eval_color = [&](const AnimatedColorSpec& prop, ColorSpec fallback) {
             return sample_color(prop, fallback, local_t);
        };

        evaluated_proc.frequency.value = eval_scalar(spec.frequency, 1.0, "procedural_frequency");
        evaluated_proc.speed.value = eval_scalar(spec.speed, 1.0, "procedural_speed");
        evaluated_proc.amplitude.value = eval_scalar(spec.amplitude, 1.0, "procedural_amplitude");
        evaluated_proc.scale.value = eval_scalar(spec.scale, 1.0, "procedural_scale");
        evaluated_proc.angle.value = eval_scalar(spec.angle, 0.0, "procedural_angle");
        
        evaluated_proc.color_a.value = eval_color(spec.color_a, {0,0,0,255});
        evaluated_proc.color_b.value = eval_color(spec.color_b, {255,255,255,255});
        evaluated_proc.color_c.value = eval_color(spec.color_c, {0,0,0,0});
        
        evaluated_proc.spacing.value = eval_scalar(spec.spacing, 50.0, "procedural_spacing");
        evaluated_proc.border_width.value = eval_scalar(spec.border_width, 1.0, "procedural_border_width");
        evaluated_proc.border_color.value = eval_color(spec.border_color, {255,255,255,255});
        
        evaluated_proc.warp_strength.value = eval_scalar(spec.warp_strength, 0.0, "procedural_warp_strength");
        evaluated_proc.warp_frequency.value = eval_scalar(spec.warp_frequency, 5.0, "procedural_warp_frequency");
        evaluated_proc.warp_speed.value = eval_scalar(spec.warp_speed, 2.0, "procedural_warp_speed");
        
        evaluated_proc.grain_amount.value = eval_scalar(spec.grain_amount, 0.0, "procedural_grain_amount");
        evaluated_proc.grain_scale.value = eval_scalar(spec.grain_scale, 1.0, "procedural_grain_scale");
        evaluated_proc.scanline_intensity.value = eval_scalar(spec.scanline_intensity, 0.0, "procedural_scanline_intensity");
        evaluated_proc.scanline_frequency.value = eval_scalar(spec.scanline_frequency, 100.0, "procedural_scanline_frequency");
        evaluated_proc.contrast.value = eval_scalar(spec.contrast, 1.0, "procedural_contrast");
        evaluated_proc.gamma.value = eval_scalar(spec.gamma, 1.0, "procedural_gamma");
        evaluated_proc.saturation.value = eval_scalar(spec.saturation, 1.0, "procedural_saturation");
        evaluated_proc.softness.value = eval_scalar(spec.softness, 0.0, "procedural_softness");
        
        evaluated_proc.octave_decay.value = eval_scalar(spec.octave_decay, 0.5, "procedural_octave_decay");
        evaluated_proc.band_height.value = eval_scalar(spec.band_height, 0.5, "procedural_band_height");
        evaluated_proc.band_spread.value = eval_scalar(spec.band_spread, 1.0, "procedural_band_spread");
        
        evaluated.procedural = std::move(evaluated_proc);
    }

    evaluated.three_d = layer.three_d;

    return evaluated;
}

} // namespace tachyon::scene
