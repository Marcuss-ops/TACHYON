#pragma once

#include "tachyon/core/scene/evaluator.h"
#include "tachyon/audio/audio_analyzer.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"

#include <string>
#include <unordered_map>
#include <cstdint>

namespace tachyon {
namespace scene {

    std::uint64_t stable_string_hash(const std::string& text);
    std::uint64_t hash_combine(std::uint64_t lhs, std::uint64_t rhs);
    std::uint64_t make_base_expression_seed(const SceneSpec* scene, const CompositionSpec& composition);
    std::uint64_t make_property_expression_seed(
        const SceneSpec* scene,
        const CompositionSpec& composition,
        const LayerSpec& layer,
        const char* property_name);

    std::string trim_copy(const std::string& text);
    std::string resolve_template(
        const std::string& tmpl,
        const std::unordered_map<std::string, std::string>* str_vars,
        const std::unordered_map<std::string, double>* num_vars);

    math::Vector2 fallback_position(const LayerSpec& layer);
    math::Vector2 fallback_scale(const LayerSpec& layer);
    double fallback_rotation(const LayerSpec& layer);

    double sample_scalar(
        const AnimatedScalarSpec& property,
        double fallback,
        double local_time_seconds,
        const ::tachyon::audio::AudioAnalyzer* audio_analyzer = nullptr,
        std::uint64_t expression_seed = 0,
        const std::unordered_map<std::string, double>* job_variables = nullptr);

    math::Vector2 sample_vector2(
        const AnimatedVector2Spec& property,
        const math::Vector2& fallback,
        double local_time_seconds,
        const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
        std::uint64_t expression_seed = 0,
        const std::unordered_map<std::string, double>* job_variables = nullptr);

    math::Vector3 sample_vector3(
        const AnimatedVector3Spec& property,
        const math::Vector3& fallback,
        double local_time_seconds,
        const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
        std::uint64_t expression_seed = 0,
        const std::unordered_map<std::string, double>* job_variables = nullptr);

    ColorSpec sample_color(const AnimatedColorSpec& property, const ColorSpec& fallback, double local_time_seconds);

} // namespace scene
} // namespace tachyon