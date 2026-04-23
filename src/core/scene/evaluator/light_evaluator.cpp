#include "tachyon/core/scene/evaluator/light_evaluator.h"
#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/core/scene/evaluator/hashing.h"

namespace tachyon::scene {

// Default values for light properties (replacing magic numbers)
namespace { // anonymous namespace for constants
    constexpr float DEFAULT_LIGHT_INTENSITY = 1.0f;
    constexpr float DEFAULT_ATTENUATION_NEAR = 0.0f;
    constexpr float DEFAULT_ATTENUATION_FAR = 1000.0f;
    constexpr float DEFAULT_CONE_ANGLE = 90.0f;
    constexpr float DEFAULT_CONE_FEATHER = 50.0f;
    constexpr float DEFAULT_SHADOW_DARKNESS = 1.0f;
    constexpr float DEFAULT_SHADOW_RADIUS = 0.0f;
    constexpr std::uint8_t DEFAULT_COLOR_COMPONENT = 255;
    constexpr float COLOR_NORMALIZE_FACTOR = 255.0f;
    constexpr math::Vector3 DEFAULT_LIGHT_DIRECTION_FORWARD{0.0f, 0.0f, -1.0f};
} // namespace

EvaluatedLightState evaluate_light_state(
    const EvaluatedLayerState& layer_state,
    const LayerSpec& spec,
    double remapped_time) {
    EvaluatedLightState light;
    light.layer_id = layer_state.id;
    light.type = spec.light_type;
    light.position = layer_state.world_position3;
    
    const auto forward = layer_state.world_matrix.transform_vector(DEFAULT_LIGHT_DIRECTION_FORWARD).normalized();
    light.direction = forward;
    
    const std::uint64_t light_seed = hash_combine(stable_string_hash(layer_state.id), stable_string_hash(light.type));
    ColorSpec c = sample_color(spec.light_color, {DEFAULT_COLOR_COMPONENT, DEFAULT_COLOR_COMPONENT, DEFAULT_COLOR_COMPONENT, DEFAULT_COLOR_COMPONENT}, remapped_time);
    light.color = math::Vector3{c.r / COLOR_NORMALIZE_FACTOR, c.g / COLOR_NORMALIZE_FACTOR, c.b / COLOR_NORMALIZE_FACTOR};
    light.intensity = static_cast<float>(sample_scalar(spec.light_intensity, DEFAULT_LIGHT_INTENSITY, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("intensity"))));
    light.attenuation_near = static_cast<float>(sample_scalar(spec.attenuation_near, DEFAULT_ATTENUATION_NEAR, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("attenuation_near"))));
    light.attenuation_far = static_cast<float>(sample_scalar(spec.attenuation_far, DEFAULT_ATTENUATION_FAR, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("attenuation_far"))));
    
    light.cone_angle = static_cast<float>(sample_scalar(spec.cone_angle, DEFAULT_CONE_ANGLE, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("cone_angle"))));
    light.cone_feather = static_cast<float>(sample_scalar(spec.cone_feather, DEFAULT_CONE_FEATHER, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("cone_feather"))));
    light.falloff_type = spec.falloff_type;
    light.casts_shadows = spec.casts_shadows;
    light.shadow_darkness = static_cast<float>(sample_scalar(spec.shadow_darkness, DEFAULT_SHADOW_DARKNESS, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("shadow_darkness"))));
    light.shadow_radius = static_cast<float>(sample_scalar(spec.shadow_radius, DEFAULT_SHADOW_RADIUS, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("shadow_radius"))));

    return light;
}

} // namespace tachyon::scene
