#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/audio/audio_analyzer.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"

#include <unordered_map>
#include <vector>
#include <string>

namespace tachyon::scene {

TACHYON_API double sample_scalar(
    const AnimatedScalarSpec& property,
    double fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer = nullptr,
    std::uint64_t expression_seed = 0,
    const std::unordered_map<std::string, double>* job_variables = nullptr,
    const std::unordered_map<std::string, std::vector<std::vector<std::string>>>* tables = nullptr,
    std::uint32_t layer_index = 0,
    PropertySampler sampler = nullptr,
    bool skip_expression = false);

TACHYON_API math::Vector2 sample_vector2(
    const AnimatedVector2Spec& property,
    const math::Vector2& fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer = nullptr,
    std::uint64_t expression_seed = 0,
    const std::unordered_map<std::string, double>* job_variables = nullptr,
    const std::unordered_map<std::string, std::vector<std::vector<std::string>>>* tables = nullptr);

TACHYON_API math::Vector3 sample_vector3(
    const AnimatedVector3Spec& property,
    const math::Vector3& fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer = nullptr,
    std::uint64_t expression_seed = 0,
    const std::unordered_map<std::string, double>* job_variables = nullptr,
    const std::unordered_map<std::string, std::vector<std::vector<std::string>>>* tables = nullptr);

TACHYON_API ColorSpec sample_color(const AnimatedColorSpec& property, const ColorSpec& fallback, double local_time_seconds);

} // namespace tachyon::scene
