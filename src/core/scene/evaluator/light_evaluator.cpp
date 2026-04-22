#include "tachyon/core/scene/evaluator/light_evaluator.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/core/scene/evaluator/hashing.h"

namespace tachyon::scene {

EvaluatedLightState evaluate_light_state(
    const EvaluatedLayerState& layer_state,
    const LayerSpec& spec,
    double remapped_time) {
    EvaluatedLightState light;
    light.layer_id = layer_state.id;
    light.type = spec.light_type.value_or("point");
    light.position = layer_state.world_position3;
    
    const auto forward = layer_state.world_matrix.transform_vector({0.0f, 0.0f, -1.0f}).normalized();
    light.direction = forward;
    
    const std::uint64_t light_seed = hash_combine(stable_string_hash(layer_state.id), stable_string_hash(light.type));
    light.color = sample_color(spec.fill_color, {255, 255, 255, 255}, remapped_time);
    light.intensity = static_cast<float>(sample_scalar(spec.intensity, 1.0, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("intensity"))));
    light.attenuation_near = static_cast<float>(sample_scalar(spec.attenuation_near, 0.0, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("attenuation_near"))));
    light.attenuation_far = static_cast<float>(sample_scalar(spec.attenuation_far, 1000.0, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("attenuation_far"))));
    
    light.cone_angle = static_cast<float>(sample_scalar(spec.cone_angle, 90.0, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("cone_angle"))));
    light.cone_feather = static_cast<float>(sample_scalar(spec.cone_feather, 50.0, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("cone_feather"))));
    light.falloff_type = spec.falloff_type;
    light.casts_shadows = spec.casts_shadows;
    light.shadow_darkness = static_cast<float>(sample_scalar(spec.shadow_darkness, 1.0, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("shadow_darkness"))));
    light.shadow_radius = static_cast<float>(sample_scalar(spec.shadow_radius, 0.0, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("shadow_radius"))));

    return light;
}

} // namespace tachyon::scene
