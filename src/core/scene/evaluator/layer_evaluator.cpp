#include "tachyon/core/scene/evaluator/layer_evaluator.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/core/scene/evaluator/layer_utils.h"
#include "tachyon/core/scene/math/evaluator_math.h"
#include "tachyon/core/scene/evaluator/hashing.h"

#include <algorithm>
#include <cmath>

namespace tachyon::scene {

EvaluatedLayerState make_layer_state(
    EvaluationContext& context,
    const LayerSpec& layer,
    std::size_t layer_index,
    double time_offset) {

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
    
    evaluated.width = layer.width;
    evaluated.height = layer.height;
    evaluated.blend_mode = layer.blend_mode;

    const double t = context.composition_time_seconds + time_offset;
    const double local_t = t - layer.start_time;
    evaluated.local_time_seconds = local_t;
    evaluated.child_time_seconds = local_t; // Simplified, in-point would subtract here if needed

    evaluated.active = layer.enabled && (t >= layer.in_point && t < layer.out_point);

    // Sample Properties
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

    evaluated.local_transform.position = pos2;
    evaluated.local_transform.rotation_rad = static_cast<float>(degrees_to_radians(rot_deg));
    evaluated.local_transform.scale = scale2;
    evaluated.world_position3 = {pos2.x, pos2.y, 0.0f};
    evaluated.scale_3d = {scale2.x, scale2.y, 1.0f};
    evaluated.world_matrix = math::compose_trs(
        evaluated.world_position3,
        math::Quaternion::from_euler({0.0f, 0.0f, static_cast<float>(rot_deg)}),
        evaluated.scale_3d);

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
    evaluated.text_content = layer.text_content;
    evaluated.font_id = layer.font_id;
    evaluated.font_size = static_cast<float>(sample_scalar(layer.font_size, 24.0, local_t, context.audio_analyzer));
    evaluated.fill_color = sample_color(layer.fill_color, {255,255,255,255}, local_t);
    evaluated.stroke_color = sample_color(layer.stroke_color, {0,0,0,255}, local_t);
    evaluated.stroke_width = static_cast<float>(sample_scalar(layer.stroke_width_property, layer.stroke_width, local_t, context.audio_analyzer));

    // Previous state for motion blur
    const double frame_duration = 1.0 / context.composition.frame_rate.value();
    const double prev_local_t = (t - frame_duration) - layer.start_time;
    const math::Vector2 prev_pos2 = sample_vector2(layer.transform.position_property, position_fallback, prev_local_t, context.audio_analyzer);
    const double prev_rot_deg = sample_scalar(layer.transform.rotation_property, layer.transform.rotation.value_or(0.0), prev_local_t, context.audio_analyzer);
    const math::Vector2 prev_scale2 = sample_vector2(layer.transform.scale_property, scale_fallback, prev_local_t, context.audio_analyzer);
    evaluated.previous_world_matrix = math::compose_trs(
        {prev_pos2.x, prev_pos2.y, 0.0f},
        math::Quaternion::from_euler({0.0f, 0.0f, static_cast<float>(prev_rot_deg)}),
        {prev_scale2.x, prev_scale2.y, 1.0f});

    return evaluated;
}

} // namespace tachyon::scene
