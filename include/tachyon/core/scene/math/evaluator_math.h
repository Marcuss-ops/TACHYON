#pragma once

#include "tachyon/core/math/algebra/vector2.h"
#include "tachyon/core/math/algebra/vector3.h"
#include "tachyon/core/math/algebra/matrix4x4.h"
#include "tachyon/core/math/algebra/quaternion.h"
#include "tachyon/core/math/geometry/transform2.h"
#include "tachyon/core/animation/easing.h"
#include "tachyon/core/expressions/expression_engine.h"
#include "tachyon/audio/processing/audio_analyzer.h"
#include "tachyon/timeline/time.h"

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
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer = nullptr);

template <typename Spec>
math::Vector2 sample_vector2(
    const Spec& property,
    const math::Vector2& fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer = nullptr);

template <typename Spec>
math::Vector3 sample_vector3(
    const Spec& property,
    const math::Vector3& fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer = nullptr);

math::Transform2 make_transform2(
    const math::Vector2& position,
    double rotation_degrees,
    const math::Vector2& scale);

} // namespace
} // namespace scene


