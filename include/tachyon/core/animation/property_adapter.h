#pragma once

#include "tachyon/core/animation/animated_property.h"
#include "tachyon/core/spec/schema/properties/property_spec.h"
#include "tachyon/runtime/core/data/compiled_scene.h"

namespace tachyon::animation {

/**
 * @brief Converts an AnimatedScalarSpec to the generic AnimatedProperty template.
 */
inline AnimatedProperty<double> to_generic(const AnimatedScalarSpec& spec) {
    AnimatedProperty<double> prop;
    prop.value = spec.value;
    prop.expression = spec.expression;
    prop.keyframes.reserve(spec.keyframes.size());
    for (const auto& k : spec.keyframes) {
        KeyframeSpec<double> gk;
        gk.time = k.time;
        gk.value = k.value;
        gk.timing.easing = k.easing;
        gk.timing.interpolation = k.interpolation;
        gk.timing.bezier = k.bezier;
        gk.timing.spring.stiffness = k.spring.stiffness;
        gk.timing.spring.damping = k.spring.damping;
        gk.timing.spring.mass = k.spring.mass;
        gk.timing.spring.velocity = k.spring.velocity;
        gk.timing.speed_in = k.speed_in;
        gk.timing.influence_in = k.influence_in;
        gk.timing.speed_out = k.speed_out;
        gk.timing.influence_out = k.influence_out;
        prop.keyframes.push_back(gk);
    }
    return prop;
}

/**
 * @brief Converts an AnimatedColorSpec to the generic AnimatedProperty template.
 */
inline AnimatedProperty<ColorSpec> to_generic(const AnimatedColorSpec& spec) {
    AnimatedProperty<ColorSpec> prop;
    prop.value = spec.value;
    prop.expression = spec.expression;
    prop.keyframes.reserve(spec.keyframes.size());
    for (const auto& k : spec.keyframes) {
        KeyframeSpec<ColorSpec> gk;
        gk.time = k.time;
        gk.value = k.value;
        gk.timing.easing = k.easing;
        gk.timing.interpolation = k.interpolation;
        gk.timing.bezier = k.bezier;
        gk.timing.speed_in = k.speed_in;
        gk.timing.influence_in = k.influence_in;
        gk.timing.speed_out = k.speed_out;
        gk.timing.influence_out = k.influence_out;
        prop.keyframes.push_back(gk);
    }
    return prop;
}

/**
 * @brief Converts an AnimatedVector2Spec to the generic AnimatedProperty template.
 */
inline AnimatedProperty<math::Vector2> to_generic(const AnimatedVector2Spec& spec) {
    AnimatedProperty<math::Vector2> prop;
    prop.value = spec.value;
    prop.expression = spec.expression;
    prop.keyframes.reserve(spec.keyframes.size());
    for (const auto& k : spec.keyframes) {
        KeyframeSpec<math::Vector2> gk;
        gk.time = k.time;
        gk.value = k.value;
        gk.timing.easing = k.easing;
        gk.timing.interpolation = k.interpolation;
        gk.timing.bezier = k.bezier;
        gk.timing.spring.stiffness = k.spring.stiffness;
        gk.timing.spring.damping = k.spring.damping;
        gk.timing.spring.mass = k.spring.mass;
        gk.timing.spring.velocity = k.spring.velocity;
        gk.timing.speed_in = k.speed_in;
        gk.timing.influence_in = k.influence_in;
        gk.timing.speed_out = k.speed_out;
        gk.timing.influence_out = k.influence_out;
        prop.keyframes.push_back(gk);
    }
    return prop;
}

/**
 * @brief Converts an AnimatedVector3Spec to the generic AnimatedProperty template.
 */
inline AnimatedProperty<math::Vector3> to_generic(const AnimatedVector3Spec& spec) {
    AnimatedProperty<math::Vector3> prop;
    prop.value = spec.value;
    prop.expression = spec.expression;
    prop.keyframes.reserve(spec.keyframes.size());
    for (const auto& k : spec.keyframes) {
        KeyframeSpec<math::Vector3> gk;
        gk.time = k.time;
        gk.value = k.value;
        gk.timing.easing = k.easing;
        gk.timing.interpolation = k.interpolation;
        gk.timing.bezier = k.bezier;
        gk.timing.speed_in = k.speed_in;
        gk.timing.influence_in = k.influence_in;
        gk.timing.speed_out = k.speed_out;
        gk.timing.influence_out = k.influence_out;
        prop.keyframes.push_back(gk);
    }
    return prop;
}

/**
 * @brief Converts a CompiledPropertyTrack to the generic AnimatedProperty template.
 */
inline AnimatedProperty<double> to_generic(const CompiledPropertyTrack& track) {
    AnimatedProperty<double> prop;
    prop.value = track.constant_value;
    prop.keyframes.reserve(track.keyframes.size());
    for (const auto& k : track.keyframes) {
        KeyframeSpec<double> gk;
        gk.time = k.time;
        gk.value = k.value;
        gk.timing.easing = static_cast<animation::EasingPreset>(k.easing);
        gk.timing.interpolation = animation::InterpolationMode::Linear; // Default for compiled
        gk.timing.bezier.cx1 = k.cx1;
        gk.timing.bezier.cy1 = k.cy1;
        gk.timing.bezier.cx2 = k.cx2;
        gk.timing.bezier.cy2 = k.cy2;
        gk.timing.spring.stiffness = k.spring_stiffness;
        gk.timing.spring.damping = k.spring_damping;
        gk.timing.spring.mass = k.spring_mass;
        prop.keyframes.push_back(gk);
    }
    return prop;
}

} // namespace tachyon::animation
