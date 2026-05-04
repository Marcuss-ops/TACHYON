#include "tachyon/core/scene/evaluator/property_sampler.h"
#include "tachyon/core/animation/property_sampler.h"
#include "tachyon/core/animation/property_interpolation.h"
#include "tachyon/core/animation/property_adapter.h"
#include "tachyon/core/expressions/context_builder.h"
#include "tachyon/audio/audio_analyzer.h"
#include "tachyon/renderer2d/animation/easing.h"
#include "tachyon/renderer2d/math/math_utils.h"
#include <algorithm>
#include <cstdlib>
#include <cmath>

namespace tachyon::scene {

namespace {

constexpr float kTangentEpsilon = 1.0e-6f;

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
        expressions::ExpressionContextInput input;
        input.time = local_time_seconds;
        input.seed = expression_seed;
        input.layer_index = layer_index;
        input.fallback_value = fallback;
        input.audio_analyzer = audio_analyzer;
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

    if (property.audio_band.has_value() && audio_analyzer) {
        const ::tachyon::audio::AudioBands bands = audio_analyzer->analyze_frame(local_time_seconds);
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

    auto generic_prop = animation::to_generic(property);
    return animation::sample_keyframes(generic_prop.keyframes, property.value.value_or(fallback), local_time_seconds, animation::lerp_scalar);
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
        expressions::ExpressionContextInput input;
        input.time = local_time_seconds;
        input.seed = expression_seed;
        input.fallback_value = 0.0; // Vector fallback logic is complex
        input.audio_analyzer = audio_analyzer;
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

    auto generic_prop = animation::to_generic(property);
    
    // Check if any interval has spatial tangents
    bool has_spatial = false;
    for (const auto& k : property.keyframes) {
        if (k.tangent_out.length_squared() > kTangentEpsilon) { has_spatial = true; break; }
        // Note: we'd also need to check the next keyframe's tangent_in, but if any tangent is non-zero, we might need spatial.
    }
    // A better check: find the interval first.

    // For now, if no tangents are used, use the generic sampler.
    // If tangents are used, we still use the legacy logic until the generic sampler supports spatial tangents.
    bool uses_tangents = false;
    for (const auto& k : property.keyframes) {
        if (k.tangent_in.length_squared() > kTangentEpsilon || k.tangent_out.length_squared() > kTangentEpsilon) {
            uses_tangents = true;
            break;
        }
    }

    if (!uses_tangents) {
        return animation::sample_keyframes(generic_prop.keyframes, property.value.value_or(fallback), local_time_seconds, animation::lerp_vector2);
    }

    // Fallback to legacy logic for spatial tangents
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
        expressions::ExpressionContextInput input;
        input.time = local_time_seconds;
        input.seed = expression_seed;
        input.audio_analyzer = audio_analyzer;
        input.job_variables = job_variables;
        input.tables = tables;
        
        if (!property.keyframes.empty()) {
            input.prop_start = property.keyframes.front().time;
            input.prop_end = property.keyframes.back().time;
        }

        auto expr_ctx = expressions::build_context(input);
        auto result = expressions::CoreExpressionEvaluator::evaluate(*property.expression, expr_ctx);
        if (result.success) {
            return { static_cast<float>(result.value), static_cast<float>(result.value), static_cast<float>(result.value) };
        }
    }

    if (property.keyframes.empty()) {
        return property.value.value_or(fallback);
    }

    auto generic_prop = animation::to_generic(property);

    bool uses_tangents = false;
    for (const auto& k : property.keyframes) {
        if (k.tangent_in.length_squared() > kTangentEpsilon || k.tangent_out.length_squared() > kTangentEpsilon) {
            uses_tangents = true;
            break;
        }
    }

    if (!uses_tangents) {
        return animation::sample_keyframes(generic_prop.keyframes, property.value.value_or(fallback), local_time_seconds, animation::lerp_vector3);
    }

    // Fallback to legacy logic for spatial tangents
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

    auto generic_prop = animation::to_generic(property);
    return animation::sample_keyframes(generic_prop.keyframes, property.value.value_or(fallback), local_time_seconds, animation::lerp_color);
}

} // namespace tachyon::scene
