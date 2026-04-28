#pragma once

#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/audio/processing/audio_analyzer.h"
#include "tachyon/core/math/algebra/vector2.h"
#include "tachyon/core/math/algebra/vector3.h"

#include <unordered_map>
#include <vector>
#include <string>

#include <nlohmann/json.hpp>
#include <map>

namespace tachyon::scene {

double sample_scalar(
    const AnimatedScalarSpec& property,
    double fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer = nullptr,
    std::uint64_t expression_seed = 0,
    const std::unordered_map<std::string, double>* job_variables = nullptr,
    const std::unordered_map<std::string, std::vector<std::vector<std::string>>>* tables = nullptr,
    std::uint32_t layer_index = 0,
    PropertySampler sampler = nullptr,
    bool skip_expression = false,
    const std::map<std::string, nlohmann::json>* input_props = nullptr);

math::Vector2 sample_vector2(
    const AnimatedVector2Spec& property,
    const math::Vector2& fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer = nullptr,
    std::uint64_t expression_seed = 0,
    const std::unordered_map<std::string, double>* job_variables = nullptr,
    const std::unordered_map<std::string, std::vector<std::vector<std::string>>>* tables = nullptr,
    std::uint32_t layer_index = 0,
    PropertySampler sampler = nullptr,
    bool skip_expression = false,
    const std::map<std::string, nlohmann::json>* input_props = nullptr);

math::Vector3 sample_vector3(
    const AnimatedVector3Spec& property,
    const math::Vector3& fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer = nullptr,
    std::uint64_t expression_seed = 0,
    const std::unordered_map<std::string, double>* job_variables = nullptr,
    const std::unordered_map<std::string, std::vector<std::vector<std::string>>>* tables = nullptr,
    std::uint32_t layer_index = 0,
    PropertySampler sampler = nullptr,
    bool skip_expression = false,
    const std::map<std::string, nlohmann::json>* input_props = nullptr);

ColorSpec sample_color(const AnimatedColorSpec& property, const ColorSpec& fallback, double local_time_seconds);

} // namespace tachyon::scene


