#include "tachyon/core/scene/evaluator/layer_evaluator.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/core/scene/evaluator/layer_utils.h"
#include "tachyon/core/scene/evaluator/templates.h"
#include "tachyon/core/scene/math/evaluator_math.h"
#include "tachyon/core/scene/evaluator/hashing.h"
#include "tachyon/core/scene/evaluator/camera2d_evaluator.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <iostream>

namespace tachyon::scene {

EvaluatedLayerState make_layer_state(
    EvaluationContext& context,
    const LayerSpec& layer,
    std::size_t layer_index,
    double time_offset,
    const EvaluationVariables& vars) {

    EvaluatedLayerState evaluated;
    if (const char* diagnostics = std::getenv("TACHYON_DIAGNOSTICS");
        diagnostics != nullptr && diagnostics[0] != '\0' && diagnostics[0] != '0') {
        std::cerr << "[LayerEvaluator] layer.id='" << layer.id
                  << "', name='" << layer.name << "'" << std::endl;
    }
    
    evaluated.layer_id = layer.id;
    evaluated.id = layer.id;
    evaluated.name = layer.name;
    evaluated.asset_path = layer.asset_id;
    evaluated.layer_index = layer_index;
    evaluated.type = layer.type;
    evaluated.enabled = layer.enabled;
    evaluated.visible = layer.visible;
    evaluated.is_adjustment_layer = layer.is_adjustment_layer;
    evaluated.motion_blur = layer.motion_blur;
    evaluated.track_matte_type = layer.track_matte_type;
    evaluated.precomp_id = layer.precomp_id;
    
    evaluated.width = layer.width;
    evaluated.height = layer.height;
    evaluated.blend_mode = layer.blend_mode;

    const double t = context.composition_time_seconds + time_offset;
    double local_t = t - layer.timing.start;

    // Apply loop and hold_last_frame behavior
    const double layer_duration = layer.timing.duration;
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

    evaluated.active = layer.enabled && (t >= layer.timing.start && t < layer.timing.start + layer.timing.duration);
    const double frame_duration = 1.0 / context.composition.frame_rate.value();
    double prev_local_t = (t - frame_duration) - layer.timing.start;

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

    const math::Vector2 anchor2 = sample_vector2(layer.transform.anchor_point,
        math::Vector2{0.0f, 0.0f},
        local_t, context.audio_analyzer);

    evaluated.local_transform.position = pos2;
    evaluated.local_transform.rotation_rad = static_cast<float>(degrees_to_radians(rot_deg));
    evaluated.local_transform.scale = scale2;
    evaluated.local_transform.anchor_point = anchor2;

    evaluated.world_matrix = math::Matrix3x3::make_translation(pos2) * 
                             math::Matrix3x3::make_rotation(evaluated.local_transform.rotation_rad) * 
                             math::Matrix3x3::make_scale(scale2.x, scale2.y) *
                             math::Matrix3x3::make_translation(anchor2 * -1.0f);

    const math::Vector2 prev_pos2 = sample_vector2(layer.transform.position_property, position_fallback, prev_local_t, context.audio_analyzer);
    const double prev_rot_deg = sample_scalar(layer.transform.rotation_property, layer.transform.rotation.value_or(0.0), prev_local_t, context.audio_analyzer);
    const math::Vector2 prev_scale2 = sample_vector2(layer.transform.scale_property, scale_fallback, prev_local_t, context.audio_analyzer);

    evaluated.previous_world_matrix = math::Matrix3x3::make_translation(prev_pos2) * 
                                      math::Matrix3x3::make_rotation(static_cast<float>(degrees_to_radians(prev_rot_deg))) * 
                                      math::Matrix3x3::make_scale(prev_scale2.x, prev_scale2.y);

    if (layer.has_parallax && !context.composition.cameras_2d.empty()) {
        std::string camera_id = layer.camera2d_id.value_or(context.composition.active_camera2d_id.value_or(""));
        if (!camera_id.empty()) {
            auto it = context.camera2d_indices.find(camera_id);
            if (it != context.camera2d_indices.end()) {
                const Camera2DSpec& camera_spec = context.composition.cameras_2d[it->second];
                EvaluatedCamera2D camera = evaluate_camera2d(camera_spec, local_t);
                math::Vector2 transformed_pos = apply_camera2d_transform(camera, layer, layer.parallax_factor, pos2);
                evaluated.world_matrix = math::Matrix3x3::make_translation(transformed_pos) * 
                                         math::Matrix3x3::make_rotation(evaluated.local_transform.rotation_rad) * 
                                         math::Matrix3x3::make_scale(scale2.x, scale2.y) *
                                         math::Matrix3x3::make_translation(anchor2 * -1.0f);
                evaluated.local_transform.position = transformed_pos;
            }
        }
    }

    evaluated.transition_in = layer.transition_in;
    evaluated.transition_out = layer.transition_out;
    evaluated.in_time = layer.timing.start;
    evaluated.out_time = layer.timing.start + layer.timing.duration;

    // Text specific
    evaluated.text_content = resolve_template(layer.text_content, vars.strings, vars.numeric);
    evaluated.font_id = layer.font_id;
    evaluated.font_size = static_cast<float>(sample_scalar(layer.font_size, 24.0, local_t, context.audio_analyzer));
    evaluated.fill_color = sample_color(layer.fill_color, {255,255,255,255}, local_t);
    evaluated.stroke_color = sample_color(layer.stroke_color, {0,0,0,255}, local_t);
    evaluated.stroke_width = static_cast<float>(sample_scalar(layer.stroke_width_property, layer.stroke_width, local_t, context.audio_analyzer));
    evaluated.text_animators = layer.text_animators;
    evaluated.text_highlights = layer.text_highlights;

    evaluated.effects.reserve(layer.effects.size());
    for (const auto& effect : layer.effects) {
        evaluated.effects.push_back(effect.evaluate(local_t));
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

    return evaluated;
}

} // namespace tachyon::scene
