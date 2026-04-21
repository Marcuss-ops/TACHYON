#include "tachyon/core/scene/evaluator_util.h"
#include "tachyon/renderer2d/animation/easing.h"
#include "tachyon/core/math/quaternion.h"
#include "tachyon/core/math/transform2.h"
#include "tachyon/renderer2d/expressions/expression_evaluator.h"
#include "tachyon/renderer2d/math/math_utils.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/renderer2d/audio/audio_sampling.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

namespace tachyon {
namespace scene {


std::uint64_t stable_string_hash(const std::string& text) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (unsigned char ch : text) {
        hash ^= static_cast<std::uint64_t>(ch);
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::uint64_t hash_combine(std::uint64_t lhs, std::uint64_t rhs) {
    lhs ^= rhs + 0x9E3779B97F4A7C15ULL + (lhs << 6) + (lhs >> 2);
    return lhs;
}

std::uint64_t make_base_expression_seed(const SceneSpec* scene, const CompositionSpec& composition) {
    std::uint64_t seed = scene && scene->project.root_seed.has_value()
        ? static_cast<std::uint64_t>(*scene->project.root_seed)
        : 0x6D6574616C736565ULL;
    seed = hash_combine(seed, stable_string_hash(composition.id));
    seed = hash_combine(seed, stable_string_hash(composition.name));
    return seed;
}

std::uint64_t make_property_expression_seed(
    const SceneSpec* scene,
    const CompositionSpec& composition,
    const LayerSpec& layer,
    const char* property_name) {
    std::uint64_t seed = make_base_expression_seed(scene, composition);
    seed = hash_combine(seed, stable_string_hash(layer.id));
    seed = hash_combine(seed, stable_string_hash(layer.name));
    seed = hash_combine(seed, stable_string_hash(property_name));
    return seed;
}

std::string trim_copy(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string resolve_template(
    const std::string& tmpl,
    const std::unordered_map<std::string, std::string>* str_vars,
    const std::unordered_map<std::string, double>* num_vars) {
    if (tmpl.find("{{") == std::string::npos) return tmpl;

    std::string result;
    result.reserve(tmpl.size());
    std::size_t cursor = 0;
    while (cursor < tmpl.size()) {
        const std::size_t open = tmpl.find("{{", cursor);
        if (open == std::string::npos) {
            result.append(tmpl, cursor, std::string::npos);
            break;
        }

        result.append(tmpl, cursor, open - cursor);
        const std::size_t close = tmpl.find("}}", open + 2);
        if (close == std::string::npos) {
            result.append(tmpl, open, std::string::npos);
            break;
        }

        const std::string key = trim_copy(tmpl.substr(open + 2, close - (open + 2)));
        bool found = false;
        if (str_vars) {
            const auto sit = str_vars->find(key);
            if (sit != str_vars->end()) {
                result += sit->second;
                found = true;
            }
        }
        if (!found && num_vars) {
            const auto nit = num_vars->find(key);
            if (nit != num_vars->end()) {
                auto d = nit->second;
                if (d == static_cast<long long>(d)) {
                    result += std::to_string(static_cast<long long>(d));
                } else {
                    result += std::to_string(d);
                }
                found = true;
            }
        }

        if (!found) {
            result.append(tmpl, open, close - open + 2);
        }
        cursor = close + 2;
    }
    return result;
}

math::Vector2 fallback_position(const LayerSpec& layer) {
    return {
        static_cast<float>(layer.transform.position_x.value_or(0.0)),
        static_cast<float>(layer.transform.position_y.value_or(0.0))
    };
}

math::Vector2 fallback_scale(const LayerSpec& layer) {
    return {
        static_cast<float>(layer.transform.scale_x.value_or(1.0)),
        static_cast<float>(layer.transform.scale_y.value_or(1.0))
    };
}

double fallback_rotation(const LayerSpec& layer) {
    return layer.transform.rotation.value_or(0.0);
}

double sample_scalar(
    const AnimatedScalarSpec& property,
    double fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
    std::uint64_t expression_seed,
    const std::unordered_map<std::string, double>* job_variables,
    const std::unordered_map<std::string, std::vector<std::vector<std::string>>>* tables) {

    if (property.expression.has_value() && !property.expression->empty()) {
        renderer2d::expressions::ExpressionContext expr_ctx;
        if (job_variables) {
            for (const auto& [k, v] : *job_variables) {
                expr_ctx.variables.try_emplace(k, v);
            }
        }
        expr_ctx.variables["t"] = local_time_seconds;
        expr_ctx.variables["time"] = local_time_seconds;
        expr_ctx.variables["value"] = fallback;
        expr_ctx.seed = expression_seed;
        expr_ctx.variables["seed"] = static_cast<double>(expression_seed);
        
        if (audio_analyzer) {
            const ::tachyon::audio::AudioBands bands = audio_analyzer->analyze_frame(local_time_seconds);
            expr_ctx.variables["music.bass"] = bands.bass;
            expr_ctx.variables["music.mid"] = bands.mid;
            expr_ctx.variables["music.high"] = bands.high;
            expr_ctx.variables["music.presence"] = bands.presence;
            expr_ctx.variables["music.rms"] = bands.rms;
        }

        if (tables) {
            expr_ctx.table_lookup = [tables](double table_idx, double row, double col) -> double {
                std::vector<std::string> keys;
                for (const auto& [k, v] : *tables) keys.push_back(k);
                std::sort(keys.begin(), keys.end());
                size_t idx = static_cast<size_t>(table_idx);
                if (idx < keys.size()) {
                    const auto& table = (*tables).at(keys[idx]);
                    size_t r = static_cast<size_t>(row);
                    size_t c = static_cast<size_t>(col);
                    if (r < table.size() && c < table[r].size()) {
                        try { return std::stod(table[r][c]); } catch (...) { return 0.0; }
                    }
                }
                return 0.0;
            };
        }
        
        auto result = renderer2d::expressions::ExpressionEvaluator::evaluate(*property.expression, expr_ctx);
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
        if (job_variables) {
            for (const auto& [k, v] : *job_variables) {
                expr_ctx.variables.try_emplace(k, v);
            }
        }
        expr_ctx.variables["t"] = local_time_seconds;
        expr_ctx.variables["time"] = local_time_seconds;
        expr_ctx.seed = expression_seed;
        expr_ctx.variables["seed"] = static_cast<double>(expression_seed);
        
        if (audio_analyzer) {
            const ::tachyon::audio::AudioBands bands = audio_analyzer->analyze_frame(local_time_seconds);
            expr_ctx.variables["music.bass"] = bands.bass;
            expr_ctx.variables["music.mid"] = bands.mid;
            expr_ctx.variables["music.high"] = bands.high;
            expr_ctx.variables["music.rms"] = bands.rms;
        }

        if (tables) {
            expr_ctx.table_lookup = [tables](double table_idx, double row, double col) -> double {
                std::vector<std::string> keys;
                for (const auto& [k, v] : *tables) keys.push_back(k);
                std::sort(keys.begin(), keys.end());
                size_t idx = static_cast<size_t>(table_idx);
                if (idx < keys.size()) {
                    const auto& table = (*tables).at(keys[idx]);
                    size_t r = static_cast<size_t>(row);
                    size_t c = static_cast<size_t>(col);
                    if (r < table.size() && c < table[r].size()) {
                        try { return std::stod(table[r][c]); } catch (...) { return 0.0; }
                    }
                }
                return 0.0;
            };
        }
        
        auto result = renderer2d::expressions::ExpressionEvaluator::evaluate(*property.expression, expr_ctx);
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
        
        if (previous.tangent_out.length_squared() > 1e-6f || next.tangent_in.length_squared() > 1e-6f) {
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
        if (job_variables) {
            for (const auto& [k, v] : *job_variables) {
                expr_ctx.variables.try_emplace(k, v);
            }
        }
        expr_ctx.variables["t"] = local_time_seconds;
        expr_ctx.variables["time"] = local_time_seconds;
        expr_ctx.seed = expression_seed;
        expr_ctx.variables["seed"] = static_cast<double>(expression_seed);
        
        if (audio_analyzer) {
            const ::tachyon::audio::AudioBands bands = audio_analyzer->analyze_frame(local_time_seconds);
            expr_ctx.variables["music.bass"] = bands.bass;
            expr_ctx.variables["music.mid"] = bands.mid;
            expr_ctx.variables["music.high"] = bands.high;
            expr_ctx.variables["music.rms"] = bands.rms;
        }

        if (tables) {
            expr_ctx.table_lookup = [tables](double table_idx, double row, double col) -> double {
                std::vector<std::string> keys;
                for (const auto& [k, v] : *tables) keys.push_back(k);
                std::sort(keys.begin(), keys.end());
                size_t idx = static_cast<size_t>(table_idx);
                if (idx < keys.size()) {
                    const auto& table = (*tables).at(keys[idx]);
                    size_t r = static_cast<size_t>(row);
                    size_t c = static_cast<size_t>(col);
                    if (r < table.size() && c < table[r].size()) {
                        try { return std::stod(table[r][c]); } catch (...) { return 0.0; }
                    }
                }
                return 0.0;
            };
        }
        
        auto result = renderer2d::expressions::ExpressionEvaluator::evaluate(*property.expression, expr_ctx);
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

        if (previous.tangent_out.length_squared() > 1e-6f || next.tangent_in.length_squared() > 1e-6f) {
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


} // namespace scene
} // namespace tachyon
