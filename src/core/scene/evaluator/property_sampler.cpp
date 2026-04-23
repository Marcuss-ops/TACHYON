#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/renderer2d/expressions/renderer2d_expression_evaluator.h"
#include "tachyon/renderer2d/audio/audio_sampling.h"
#include "tachyon/renderer2d/animation/easing.h"
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
    bool skip_expression) {

    if (!skip_expression && property.expression.has_value() && !property.expression->empty()) {
        renderer2d::expressions::ExpressionContext expr_ctx;
        expr_ctx.layer_index = layer_index;
        expr_ctx.property_sampler = sampler;

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

    const std::vector<ScalarKeyframeSpec>* keyframes = &property.keyframes;
    std::vector<ScalarKeyframeSpec> sorted_keyframes;
    if (!std::is_sorted(keyframes->begin(), keyframes->end(), [](const auto& a, const auto& b) {
            return a.time < b.time;
        })) {
        sorted_keyframes = property.keyframes;
        std::stable_sort(sorted_keyframes.begin(), sorted_keyframes.end(), [](const auto& a, const auto& b) {
            return a.time < b.time;
        });
        keyframes = &sorted_keyframes;
    }

    if (local_time_seconds <= keyframes->front().time) {
        return keyframes->front().value;
    }
    if (local_time_seconds >= keyframes->back().time) {
        return keyframes->back().value;
    }

    for (std::size_t index = 1; index < keyframes->size(); ++index) {
        const auto& previous = (*keyframes)[index - 1];
        const auto& next = (*keyframes)[index];
        if (local_time_seconds > next.time) {
            continue;
        }
        const double duration = next.time - previous.time;
        if (duration <= 0.0) {
            return next.value;
        }
        const double alpha = (local_time_seconds - previous.time) / duration;
        const double eased = renderer2d::animation::apply_easing(alpha, previous.easing, previous.bezier);
        return previous.value + (next.value - previous.value) * eased;
    }

    return keyframes->back().value;
}

math::Vector2 sample_vector2(
    const AnimatedVector2Spec& property,
    const math::Vector2& fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
    std::uint64_t expression_seed,
    const std::unordered_map<std::string, double>* job_variables,
    const std::unordered_map<std::string, std::vector<std::vector<std::string>>>* tables) {
    if (property.expression.has_value() && !property.expression->empty()) {
        renderer2d::expressions::ExpressionContext expr_ctx;
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

    const std::vector<Vector2KeyframeSpec>* keyframes = &property.keyframes;
    std::vector<Vector2KeyframeSpec> sorted_keyframes;
    if (!std::is_sorted(keyframes->begin(), keyframes->end(), [](const auto& a, const auto& b) {
            return a.time < b.time;
        })) {
        sorted_keyframes = property.keyframes;
        std::stable_sort(sorted_keyframes.begin(), sorted_keyframes.end(), [](const auto& a, const auto& b) {
            return a.time < b.time;
        });
        keyframes = &sorted_keyframes;
    }

    if (local_time_seconds <= keyframes->front().time) {
        return keyframes->front().value;
    }
    if (local_time_seconds >= keyframes->back().time) {
        return keyframes->back().value;
    }

    for (std::size_t index = 1; index < keyframes->size(); ++index) {
        const auto& previous = (*keyframes)[index - 1];
        const auto& next = (*keyframes)[index];
        if (local_time_seconds > next.time) {
            continue;
        }
        const double duration = next.time - previous.time;
        if (duration <= 0.0) {
            return next.value;
        }
        const double alpha = (local_time_seconds - previous.time) / duration;
        const double eased = renderer2d::animation::apply_easing(alpha, previous.easing, previous.bezier);
        const float weight = static_cast<float>(eased);        
        
        if (previous.tangent_out.length_squared() > kTangentEpsilon || next.tangent_in.length_squared() > kTangentEpsilon) {
            return renderer2d::math_utils::sample_bezier_spatial(
                previous.value,
                previous.value + previous.tangent_out,
                next.value + next.tangent_in,
                next.value,
                weight
            );
        }

        return previous.value * (1.0f - weight) + next.value * weight;
    }

    return keyframes->back().value;
}

math::Vector3 sample_vector3(
    const AnimatedVector3Spec& property,
    const math::Vector3& fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
    std::uint64_t expression_seed,
    const std::unordered_map<std::string, double>* job_variables,
    const std::unordered_map<std::string, std::vector<std::vector<std::string>>>* tables) {
    if (property.expression.has_value() && !property.expression->empty()) {
        renderer2d::expressions::ExpressionContext expr_ctx;
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

    const std::vector<AnimatedVector3Spec::Keyframe>* keyframes = &property.keyframes;
    std::vector<AnimatedVector3Spec::Keyframe> sorted_keyframes;
    if (!std::is_sorted(keyframes->begin(), keyframes->end(), [](const auto& a, const auto& b) {
            return a.time < b.time;
        })) {
        sorted_keyframes = property.keyframes;
        std::stable_sort(sorted_keyframes.begin(), sorted_keyframes.end(), [](const auto& a, const auto& b) {
            return a.time < b.time;
        });
        keyframes = &sorted_keyframes;
    }

    if (local_time_seconds <= keyframes->front().time) {
        return keyframes->front().value;
    }
    if (local_time_seconds >= keyframes->back().time) {
        return keyframes->back().value;
    }

    for (std::size_t index = 1; index < keyframes->size(); ++index) {
        const auto& previous = (*keyframes)[index - 1];
        const auto& next = (*keyframes)[index];
        if (local_time_seconds > next.time) {
            continue;
        }
        const double duration = next.time - previous.time;
        if (duration <= 0.0) {
            return next.value;
        }
        const double alpha = (local_time_seconds - previous.time) / duration;
        const double eased = renderer2d::animation::apply_easing(alpha, previous.easing, previous.bezier);
        const float weight = static_cast<float>(eased);

        if (previous.tangent_out.length_squared() > kTangentEpsilon || next.tangent_in.length_squared() > kTangentEpsilon) {
            return renderer2d::math_utils::sample_bezier_spatial(
                previous.value,
                previous.value + previous.tangent_out,
                next.value + next.tangent_in,
                next.value,
                weight
            );
        }

        return previous.value * (1.0f - weight) + next.value * weight;
    }

    return keyframes->back().value;
}

ColorSpec sample_color(const AnimatedColorSpec& property, const ColorSpec& fallback, double local_time_seconds) {
    if (property.keyframes.empty()) {
        return property.value.value_or(fallback);
    }

    const std::vector<ColorKeyframeSpec>* keyframes = &property.keyframes;
    std::vector<ColorKeyframeSpec> sorted_keyframes;
    if (!std::is_sorted(keyframes->begin(), keyframes->end(), [](const auto& a, const auto& b) {
            return a.time < b.time;
        })) {
        sorted_keyframes = property.keyframes;
        std::stable_sort(sorted_keyframes.begin(), sorted_keyframes.end(), [](const auto& a, const auto& b) {
            return a.time < b.time;
        });
        keyframes = &sorted_keyframes;
    }

    if (local_time_seconds <= keyframes->front().time) {
        return keyframes->front().value;
    }
    if (local_time_seconds >= keyframes->back().time) {
        return keyframes->back().value;
    }

    for (std::size_t index = 1; index < keyframes->size(); ++index) {
        const auto& previous = (*keyframes)[index - 1];
        const auto& next = (*keyframes)[index];
        if (local_time_seconds > next.time) {
            continue;
        }
        const double duration = next.time - previous.time;
        if (duration <= 0.0) {
            return next.value;
        }
        const double alpha = (local_time_seconds - previous.time) / duration;
        const double eased = renderer2d::animation::apply_easing(alpha, previous.easing, previous.bezier);
        const float weight = static_cast<float>(eased);
        
        ColorSpec result;
        result.r = static_cast<std::uint8_t>(std::clamp(previous.value.r * (1.0f - weight) + next.value.r * weight, 0.0f, 255.0f));
        result.g = static_cast<std::uint8_t>(std::clamp(previous.value.g * (1.0f - weight) + next.value.g * weight, 0.0f, 255.0f));
        result.b = static_cast<std::uint8_t>(std::clamp(previous.value.b * (1.0f - weight) + next.value.b * weight, 0.0f, 255.0f));
        result.a = static_cast<std::uint8_t>(std::clamp(previous.value.a * (1.0f - weight) + next.value.a * weight, 0.0f, 255.0f));
        return result;
    }

    return keyframes->back().value;
}

} // namespace tachyon::scene
