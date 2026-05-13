#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/math/quaternion.h"
#include "tachyon/core/math/transform2.h"
#include "tachyon/core/animation/easing.h"
#include "tachyon/core/expressions/expression_engine.h"
#include "tachyon/audio/audio_analyzer.h"
#include "tachyon/timeline/time.h"

#include <type_traits>
#include <optional>

#include "tachyon/core/spec/schema/objects/scene_spec.h"

namespace tachyon {
namespace scene {

// These functions are defined in evaluator_math.cpp
double degrees_to_radians(double degrees);

template <typename T>
T lerp(const T& a, const T& b, float t);

template <typename T>
T sample_bezier_spatial(const T& p0, const T& p1, const T& p2, const T& p3, float t);

math::Vector2 fallback_position(const LayerSpec& layer);
double fallback_rotation(const LayerSpec& layer);
math::Vector2 fallback_scale(const LayerSpec& layer);

double sample_audio_band(const ::tachyon::audio::AudioBands& bands, int band);

template <typename Spec>
double sample_scalar(
    const Spec& property,
    double fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer = nullptr) {
    if constexpr (std::is_same_v<Spec, std::optional<double>>) {
        return property.value_or(fallback);
    } else {
        // This will call the AnimatedScalarSpec overload from property_sampler.h
        return sample_scalar(property, fallback, local_time_seconds, audio_analyzer);
    }
}

template <typename Spec>
math::Vector2 sample_vector2(
    const Spec& property,
    const math::Vector2& fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer = nullptr) {
    if constexpr (std::is_same_v<Spec, std::optional<math::Vector2>>) {
        return property.value_or(fallback);
    } else {
        return sample_vector2(property, fallback, local_time_seconds, audio_analyzer);
    }
}

template <typename Spec>
math::Vector3 sample_vector3(
    const Spec& property,
    const math::Vector3& fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer = nullptr) {
    if constexpr (std::is_same_v<Spec, std::optional<math::Vector3>>) {
        return property.value_or(fallback);
    } else {
        return sample_vector3(property, fallback, local_time_seconds, audio_analyzer);
    }
}

math::Transform2 make_transform2(
    const math::Vector2& position,
    double rotation_degrees,
    const math::Vector2& scale);

} // namespace scene
} // namespace tachyon
