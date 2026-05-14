#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/core/animation/property_sampler.h"
#include "tachyon/core/animation/property_interpolation.h"
#include "tachyon/core/animation/property_adapter.h"
#include "tachyon/core/expressions/context_builder.h"
#include "tachyon/core/media/audio_types.h"
#include "tachyon/core/animation/easing.h"
#include "tachyon/core/math/math_utils.h"
#include <algorithm>
#include <cstdlib>
#include <cmath>

namespace tachyon::scene {

namespace {

constexpr float kTangentEpsilon = 1.0e-6f;

template<typename K, typename T, typename LerpFn>
T internal_sample_keyframes(
    const std::vector<K>& keyframes,
    const T& fallback,
    double time,
    LerpFn lerp) {
    if (keyframes.empty()) return fallback;
    
    if (time <= keyframes.front().time) return keyframes.front().value;
    if (time >= keyframes.back().time) return keyframes.back().value;

    auto it = std::lower_bound(keyframes.begin(), keyframes.end(), time,
        [](const K& k, double t) { return k.time < t; });

    const auto& k1 = *(it - 1);
    const auto& k2 = *it;

    if (k1.interpolation == animation::InterpolationMode::Hold) return k1.value;

    double duration = k2.time - k1.time;
    double t = (duration > 1e-8) ? (time - k1.time) / duration : 0.0;
    double remapped_t = animation::apply_easing(t, k1.easing, k1.bezier);
    return lerp(k1.value, k2.value, remapped_t);
}

template<typename K, typename T, typename LerpFn, typename BezierFn>
T internal_sample_spatial_keyframes(
    const std::vector<K>& keyframes,
    const T& fallback,
    double time,
    LerpFn lerp,
    BezierFn bezier_sample) {
    if (keyframes.empty()) return fallback;

    if (time <= keyframes.front().time) return keyframes.front().value;
    if (time >= keyframes.back().time) return keyframes.back().value;

    auto it = std::lower_bound(keyframes.begin(), keyframes.end(), time,
        [](const K& k, double t) { return k.time < t; });

    const auto& k1 = *(it - 1);
    const auto& k2 = *it;

    if (k1.interpolation == animation::InterpolationMode::Hold) return k1.value;

    double duration = k2.time - k1.time;
    double t = (duration > 1e-8) ? (time - k1.time) / duration : 0.0;
    double remapped_t = animation::apply_easing(t, k1.easing, k1.bezier);
    float weight = static_cast<float>(remapped_t);

    if (k1.tangent_out.length_squared() > kTangentEpsilon || k2.tangent_in.length_squared() > kTangentEpsilon) {
        return bezier_sample(
            k1.value,
            k1.value + k1.tangent_out,
            k2.value + k2.tangent_in,
            k2.value,
            weight
        );
    }

    return lerp(k1.value, k2.value, weight);
}

} // namespace

double sample_scalar(
    const AnimatedScalarSpec& property,
    double fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioBands& audio_bands,
    std::uint64_t expression_seed,
    const std::unordered_map<std::string, double>* job_variables,
    const std::unordered_map<std::string, std::vector<std::vector<std::string>>>* tables,
    std::uint32_t layer_index,
    PropertySampler sampler,
    bool skip_expression) {

    if (!skip_expression && property.expression.has_value() && !property.expression->empty()) {
        expressions::ExpressionContextInput input;
        input.time = local_time_seconds;
        input.seed = expression_seed;
        input.layer_index = layer_index;
        input.fallback_value = fallback;
        input.audio_bands = audio_bands;
        input.job_variables = job_variables;
        input.tables = tables;
        input.property_sampler = sampler;
        
        if (!property.keyframes.empty()) {
            input.prop_start = property.keyframes.front().time;
            input.prop_end = property.keyframes.back().time;
        }

        auto expr_ctx = expressions::build_context(input);
        auto result = expressions::CoreExpressionEvaluator::evaluate(*property.expression, expr_ctx);
        if (result.success) {
            return result.value;
        }
    }

    if (property.audio_band.has_value()) {
        const auto& bands = audio_bands;
        // Note: sample_audio_band is still in renderer2d/audio, but AudioBands is core.
        // We might want to move sample_audio_band too, but for now we keep it.
        // Actually, I'll just use the bands data directly to avoid the dependency if possible.
        double band_value = 0.0;
        switch (*property.audio_band) {
            case AudioBandType::Bass:     band_value = bands.bass; break;
            case AudioBandType::Mid:      band_value = bands.mid; break;
            case AudioBandType::High:     band_value = bands.high; break;
            case AudioBandType::Presence: band_value = bands.presence; break;
            case AudioBandType::Rms:      band_value = bands.rms; break;
        }
        const double clamped = std::clamp(band_value, 0.0, 1.0);
        return std::clamp(property.audio_min + (property.audio_max - property.audio_min) * clamped,
                          std::min(property.audio_min, property.audio_max),
                          std::max(property.audio_min, property.audio_max));
    }

    if (property.keyframes.empty()) {
        return property.value.value_or(fallback);
    }

    return internal_sample_keyframes(property.keyframes, property.value.value_or(fallback), local_time_seconds, animation::lerp_scalar);
}

math::Vector2 sample_vector2(
    const AnimatedVector2Spec& property,
    const math::Vector2& fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioBands& audio_bands,
    std::uint64_t expression_seed,
    const std::unordered_map<std::string, double>* job_variables,
    const std::unordered_map<std::string, std::vector<std::vector<std::string>>>* tables) {
    if (property.expression.has_value() && !property.expression->empty()) {
        expressions::ExpressionContextInput input;
        input.time = local_time_seconds;
        input.seed = expression_seed;
        input.fallback_value = 0.0; // Vector fallback logic is complex
        input.audio_bands = audio_bands;
        input.job_variables = job_variables;
        input.tables = tables;
        
        if (!property.keyframes.empty()) {
            input.prop_start = property.keyframes.front().time;
            input.prop_end = property.keyframes.back().time;
        }

        auto expr_ctx = expressions::build_context(input);
        auto result = expressions::CoreExpressionEvaluator::evaluate(*property.expression, expr_ctx);
        if (result.success) {
            return { static_cast<float>(result.value), static_cast<float>(result.value) };
        }
    }

    if (property.keyframes.empty()) {
        return property.value.value_or(fallback);
    }

    return internal_sample_spatial_keyframes(
        property.keyframes, 
        property.value.value_or(fallback), 
        local_time_seconds, 
        animation::lerp_vector2,
        [](const math::Vector2& p0, const math::Vector2& p1, const math::Vector2& p2, const math::Vector2& p3, float t) {
            return math::sample_bezier_spatial(p0, p1, p2, p3, t);
        });
}


ColorSpec sample_color(const AnimatedColorSpec& property, const ColorSpec& fallback, double local_time_seconds) {
    if (property.keyframes.empty()) {
        return property.value.value_or(fallback);
    }

    return internal_sample_keyframes(property.keyframes, property.value.value_or(fallback), local_time_seconds, animation::lerp_color);
}

} // namespace tachyon::scene
