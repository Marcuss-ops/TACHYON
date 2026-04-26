#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/renderer2d/expressions/renderer2d_expression_evaluator.h"
#include "tachyon/renderer2d/audio/audio_sampling.h"
#include "tachyon/core/animation/easing.h"
#include "tachyon/core/animation/animation_curve.h"
#include "tachyon/renderer2d/math/math_utils.h"
#include <algorithm>
#include <cstdlib>
#include <cmath>

namespace tachyon::scene {

namespace {

constexpr float kTangentEpsilon = 1.0e-6f;
constexpr double kDefaultTableValue = 0.0;

double parse_table_value(const std::string& value) {
    const char* begin = value.c_str();
    char* end = nullptr;
    const double parsed = std::strtod(begin, &end);
    if (end == begin || *end != '\0') {
        return kDefaultTableValue;
    }
    return parsed;
}

} // namespace

double sample_scalar(
    const AnimatedScalarSpec& property,
    double fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
    std::uint64_t expression_seed,
    const std::unordered_map<std::string, double>* job_variables,
    const std::unordered_map<std::string, std::vector<std::vector<std::string>>>* tables,
    std::uint32_t layer_index,
    PropertySampler sampler,
    bool skip_expression,
    const std::map<std::string, nlohmann::json>* input_props) {

    if (!skip_expression && property.expression.has_value() && !property.expression->empty()) {
        renderer2d::expressions::ExpressionContext expr_ctx;
        expr_ctx.time = local_time_seconds;
        expr_ctx.composition_time = local_time_seconds;
        expr_ctx.layer_index = layer_index;
        expr_ctx.property_sampler = sampler;
        if (input_props) expr_ctx.input_props = *input_props;

        std::vector<std::string> table_keys;
        if (job_variables) {
            for (const auto& [k, v] : *job_variables) {
                expr_ctx.variables.try_emplace(k, v);
            }
        }
        if (tables) {
            expr_ctx.tables = *tables;
            table_keys.reserve(tables->size());
            for (const auto& [key, _] : *tables) {
                table_keys.push_back(key);
            }
            std::sort(table_keys.begin(), table_keys.end());
        }
        expr_ctx.variables["t"] = local_time_seconds;
        expr_ctx.variables["time"] = local_time_seconds;
        expr_ctx.value = fallback;
        expr_ctx.seed = expression_seed;
        expr_ctx.variables["seed"] = static_cast<double>(expression_seed);
        renderer2d::expressions::bind_standard_expression_variables(expr_ctx);
        expr_ctx.table_lookup = [tables, table_keys](double table_idx, double row, double col) -> double {
            if (!tables || tables->empty() || table_keys.empty()) {
                return 0.0;
            }
            const auto idx = static_cast<std::size_t>(std::max(0.0, std::floor(table_idx)));
            if (idx >= table_keys.size()) {
                return 0.0;
            }
            const auto& table = tables->at(table_keys[idx]);
            const std::size_t r = static_cast<std::size_t>(std::max(0.0, std::floor(row)));
            const std::size_t c = static_cast<std::size_t>(std::max(0.0, std::floor(col)));
            if (r >= table.size() || c >= table[r].size()) {
                return 0.0;
            }
            return parse_table_value(table[r][c]);
        };
        
        if (audio_analyzer) {
            const ::tachyon::audio::AudioBands bands = audio_analyzer->analyze_frame(local_time_seconds);
            expr_ctx.variables["music.bass"] = bands.bass;
            expr_ctx.variables["music.mid"] = bands.mid;
            expr_ctx.variables["music.high"] = bands.high;
            expr_ctx.variables["music.presence"] = bands.presence;
            expr_ctx.variables["music.rms"] = bands.rms;

            // Also populate the structured audio_analysis field for newer expression access
            expr_ctx.audio_analysis = ::tachyon::audio::AudioAnalyzer::to_analysis_data(bands);
        }

        if (!property.keyframes.empty()) {
            expr_ctx.variables["_prop_start"] = property.keyframes.front().time;
            expr_ctx.variables["_prop_end"] = property.keyframes.back().time;
            expr_ctx.variables["_prop_duration"] = property.keyframes.back().time - property.keyframes.front().time;
        }
        
        auto result = renderer2d::expressions::Renderer2DExpressionEvaluator::evaluate(*property.expression, expr_ctx);
        if (result.success) {
            return result.value;
        }
    }

    if (property.audio_band.has_value() && audio_analyzer) {
        const ::tachyon::audio::AudioBands bands = audio_analyzer->analyze_frame(local_time_seconds);
        const double band_value = renderer2d::audio::sample_audio_band(bands, *property.audio_band);
        const double clamped = std::clamp(band_value, 0.0, 1.0);
        return std::clamp(property.audio_min + (property.audio_max - property.audio_min) * clamped,
                          std::min(property.audio_min, property.audio_max),
                          std::max(property.audio_min, property.audio_max));
    }

    if (property.keyframes.empty()) {
        return property.value.value_or(fallback);
    }

    animation::AnimationCurve<double> curve;
    for (const auto& kf : property.keyframes) {
        animation::Keyframe<double> akf;
        akf.time = kf.time;
        akf.value = kf.value;
        akf.out_mode = kf.interpolation;
        akf.easing = kf.easing;
        akf.bezier = kf.bezier;
        akf.spring = kf.spring;
        curve.add_keyframe(akf);
    }
    curve.sort();
    return curve.evaluate(local_time_seconds);
}

math::Vector2 sample_vector2(
    const AnimatedVector2Spec& property,
    const math::Vector2& fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
    std::uint64_t expression_seed,
    const std::unordered_map<std::string, double>* job_variables,
    const std::unordered_map<std::string, std::vector<std::vector<std::string>>>* tables,
    std::uint32_t layer_index,
    PropertySampler sampler,
    bool skip_expression,
    const std::map<std::string, nlohmann::json>* input_props) {
    if (!skip_expression && property.expression.has_value() && !property.expression->empty()) {
        renderer2d::expressions::ExpressionContext expr_ctx;
        expr_ctx.time = local_time_seconds;
        expr_ctx.composition_time = local_time_seconds;
        expr_ctx.layer_index = layer_index;
        expr_ctx.property_sampler = sampler;
        if (input_props) expr_ctx.input_props = *input_props;
        std::vector<std::string> table_keys;
        if (job_variables) {
            for (const auto& [k, v] : *job_variables) {
                expr_ctx.variables.try_emplace(k, v);
            }
        }
        if (tables) {
            expr_ctx.tables = *tables;
            table_keys.reserve(tables->size());
            for (const auto& [key, _] : *tables) {
                table_keys.push_back(key);
            }
            std::sort(table_keys.begin(), table_keys.end());
        }
        expr_ctx.variables["t"] = local_time_seconds;
        expr_ctx.variables["time"] = local_time_seconds;
        expr_ctx.seed = expression_seed;
        expr_ctx.variables["seed"] = static_cast<double>(expression_seed);
        renderer2d::expressions::bind_standard_expression_variables(expr_ctx);
        expr_ctx.table_lookup = [tables, table_keys](double table_idx, double row, double col) -> double {
            if (!tables || tables->empty() || table_keys.empty()) {
                return 0.0;
            }
            const auto idx = static_cast<std::size_t>(std::max(0.0, std::floor(table_idx)));
            if (idx >= table_keys.size()) {
                return 0.0;
            }
            const auto& table = tables->at(table_keys[idx]);
            const std::size_t r = static_cast<std::size_t>(std::max(0.0, std::floor(row)));
            const std::size_t c = static_cast<std::size_t>(std::max(0.0, std::floor(col)));
            if (r >= table.size() || c >= table[r].size()) {
                return 0.0;
            }
            return parse_table_value(table[r][c]);
        };
        
        if (audio_analyzer) {
            const ::tachyon::audio::AudioBands bands = audio_analyzer->analyze_frame(local_time_seconds);
            expr_ctx.variables["music.bass"] = bands.bass;
            expr_ctx.variables["music.mid"] = bands.mid;
            expr_ctx.variables["music.high"] = bands.high;
            expr_ctx.variables["music.presence"] = bands.presence;
            expr_ctx.variables["music.rms"] = bands.rms;

            // Also populate the structured audio_analysis field
            expr_ctx.audio_analysis = ::tachyon::audio::AudioAnalyzer::to_analysis_data(bands);
        }
        
        auto result = renderer2d::expressions::Renderer2DExpressionEvaluator::evaluate(*property.expression, expr_ctx);
        if (result.success) {
            return { static_cast<float>(result.value), static_cast<float>(result.value) };
        }
    }

    if (property.keyframes.empty()) {
        return property.value.value_or(fallback);
    }

    animation::AnimationCurve<math::Vector2> curve;
    for (const auto& kf : property.keyframes) {
        animation::Keyframe<math::Vector2> akf;
        akf.time = kf.time;
        akf.value = kf.value;
        akf.out_mode = kf.interpolation;
        akf.easing = kf.easing;
        akf.bezier = kf.bezier;
        akf.spring = kf.spring;
        
        // For Vector2, we might also have spatial tangents if out_mode is Bezier
        akf.out_tangent_value = kf.tangent_out;
        akf.in_tangent_value = kf.tangent_in;
        
        curve.add_keyframe(akf);
    }
    curve.sort();
    return curve.evaluate(local_time_seconds);
}

math::Vector3 sample_vector3(
    const AnimatedVector3Spec& property,
    const math::Vector3& fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
    std::uint64_t expression_seed,
    const std::unordered_map<std::string, double>* job_variables,
    const std::unordered_map<std::string, std::vector<std::vector<std::string>>>* tables,
    std::uint32_t layer_index,
    PropertySampler sampler,
    bool skip_expression,
    const std::map<std::string, nlohmann::json>* input_props) {
    if (!skip_expression && property.expression.has_value() && !property.expression->empty()) {
        renderer2d::expressions::ExpressionContext expr_ctx;
        expr_ctx.time = local_time_seconds;
        expr_ctx.composition_time = local_time_seconds;
        expr_ctx.layer_index = layer_index;
        expr_ctx.property_sampler = sampler;
        if (input_props) expr_ctx.input_props = *input_props;
        std::vector<std::string> table_keys;
        if (job_variables) {
            for (const auto& [k, v] : *job_variables) {
                expr_ctx.variables.try_emplace(k, v);
            }
        }
        if (tables) {
            expr_ctx.tables = *tables;
            table_keys.reserve(tables->size());
            for (const auto& [key, _] : *tables) {
                table_keys.push_back(key);
            }
            std::sort(table_keys.begin(), table_keys.end());
        }
        expr_ctx.variables["t"] = local_time_seconds;
        expr_ctx.variables["time"] = local_time_seconds;
        expr_ctx.seed = expression_seed;
        expr_ctx.variables["seed"] = static_cast<double>(expression_seed);
        renderer2d::expressions::bind_standard_expression_variables(expr_ctx);
        expr_ctx.table_lookup = [tables, table_keys](double table_idx, double row, double col) -> double {
            if (!tables || tables->empty() || table_keys.empty()) {
                return 0.0;
            }
            const auto idx = static_cast<std::size_t>(std::max(0.0, std::floor(table_idx)));
            if (idx >= table_keys.size()) {
                return 0.0;
            }
            const auto& table = tables->at(table_keys[idx]);
            const std::size_t r = static_cast<std::size_t>(std::max(0.0, std::floor(row)));
            const std::size_t c = static_cast<std::size_t>(std::max(0.0, std::floor(col)));
            if (r >= table.size() || c >= table[r].size()) {
                return 0.0;
            }
            return parse_table_value(table[r][c]);
        };
        
        if (audio_analyzer) {
            const ::tachyon::audio::AudioBands bands = audio_analyzer->analyze_frame(local_time_seconds);
            expr_ctx.variables["music.bass"] = bands.bass;
            expr_ctx.variables["music.mid"] = bands.mid;
            expr_ctx.variables["music.high"] = bands.high;
            expr_ctx.variables["music.rms"] = bands.rms;
        }
        
        auto result = renderer2d::expressions::Renderer2DExpressionEvaluator::evaluate(*property.expression, expr_ctx);
        if (result.success) {
            return { static_cast<float>(result.value), static_cast<float>(result.value), static_cast<float>(result.value) };
        }
    }

    if (property.keyframes.empty()) {
        return property.value.value_or(fallback);
    }

    animation::AnimationCurve<math::Vector3> curve;
    for (const auto& kf : property.keyframes) {
        animation::Keyframe<math::Vector3> akf;
        akf.time = kf.time;
        akf.value = kf.value;
        akf.out_mode = kf.interpolation;
        akf.easing = kf.easing;
        akf.bezier = kf.bezier;
        akf.spring = kf.spring;
        akf.out_tangent_value = kf.tangent_out;
        akf.in_tangent_value = kf.tangent_in;
        curve.add_keyframe(akf);
    }
    curve.sort();
    return curve.evaluate(local_time_seconds);
}

ColorSpec sample_color(const AnimatedColorSpec& property, const ColorSpec& fallback, double local_time_seconds) {
    if (property.keyframes.empty()) {
        return property.value.value_or(fallback);
    }

    animation::AnimationCurve<ColorSpec> curve;
    for (const auto& kf : property.keyframes) {
        animation::Keyframe<ColorSpec> akf;
        akf.time = kf.time;
        akf.value = kf.value;
        akf.out_mode = kf.interpolation;
        akf.easing = kf.easing;
        akf.bezier = kf.bezier;
        akf.spring = kf.spring;
        curve.add_keyframe(akf);
    }
    curve.sort();
    return curve.evaluate(local_time_seconds);
}

} // namespace tachyon::scene
