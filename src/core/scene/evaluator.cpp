#include "tachyon/core/scene/evaluator_math_forward.h"
#include "tachyon/core/scene/evaluator.h"
#include "tachyon/renderer2d/animation/easing.h"
#include "tachyon/core/math/quaternion.h"
#include "tachyon/core/math/transform2.h"
#include "tachyon/audio/audio_analyzer.h"
#include "tachyon/timeline/time.h"
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
namespace {

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

// Math utilities moved to renderer2d::math_utils

// Fallback functions moved to evaluator_math.cpp

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

// Audio sampling moved to renderer2d::audio

double sample_scalar(
    const AnimatedScalarSpec& property,
    double fallback,
    double local_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer = nullptr,
    std::uint64_t expression_seed = 0,
    const std::unordered_map<std::string, double>* job_variables = nullptr) {

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
        
        // Add audio bands to expression context
        if (audio_analyzer) {
            const ::tachyon::audio::AudioBands bands = audio_analyzer->analyze_frame(local_time_seconds);
            expr_ctx.variables["music.bass"] = bands.bass;
            expr_ctx.variables["music.mid"] = bands.mid;
            expr_ctx.variables["music.high"] = bands.high;
            expr_ctx.variables["music.presence"] = bands.presence;
            expr_ctx.variables["music.rms"] = bands.rms;
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
    std::uint64_t expression_seed = 0,
    const std::unordered_map<std::string, double>* job_variables = nullptr) {
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
        
        // Add audio bands to expression context
        if (audio_analyzer) {
            const ::tachyon::audio::AudioBands bands = audio_analyzer->analyze_frame(local_time_seconds);
            expr_ctx.variables["music.bass"] = bands.bass;
            expr_ctx.variables["music.mid"] = bands.mid;
            expr_ctx.variables["music.high"] = bands.high;
            expr_ctx.variables["music.rms"] = bands.rms;
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
    std::uint64_t expression_seed = 0,
    const std::unordered_map<std::string, double>* job_variables = nullptr) {
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
        
        // Add audio bands to expression context
        if (audio_analyzer) {
            const ::tachyon::audio::AudioBands bands = audio_analyzer->analyze_frame(local_time_seconds);
            expr_ctx.variables["music.bass"] = bands.bass;
            expr_ctx.variables["music.mid"] = bands.mid;
            expr_ctx.variables["music.high"] = bands.high;
            expr_ctx.variables["music.rms"] = bands.rms;
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

math::Transform2 make_transform2(const math::Vector2& position, double rotation_degrees, const math::Vector2& scale) {
    math::Transform2 transform;
    transform.position = position;
    transform.rotation_rad = static_cast<float>(renderer2d::math_utils::degrees_to_radians(rotation_degrees));
    transform.scale = scale;
    return transform;
}

LayerType map_layer_type(const std::string& type) {
    if (type == "solid") return LayerType::Solid;
    if (type == "shape") return LayerType::Shape;
    if (type == "mask") return LayerType::Mask;
    if (type == "image") return LayerType::Image;
    if (type == "video") return LayerType::Video;
    if (type == "text") return LayerType::Text;
    if (type == "camera") return LayerType::Camera;
    if (type == "precomp") return LayerType::Precomp;
    if (type == "light") return LayerType::Light;
    return LayerType::Unknown;
}

EvaluatedLayerState make_layer_state(
    const CompositionSpec& composition,
    const LayerSpec& layer,
    std::size_t layer_index,
    std::int64_t frame_number,
    double composition_time_seconds,
    const SceneSpec* scene,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars = {}) {
    (void)frame_number;
    EvaluatedLayerState evaluated;
    evaluated.layer_index = layer_index;
    evaluated.id = layer.id;
    evaluated.type = map_layer_type(layer.type);
    evaluated.name = layer.name;
    evaluated.blend_mode = layer.blend_mode;
    evaluated.enabled = layer.enabled;
    evaluated.is_3d = layer.is_3d;
    evaluated.is_adjustment_layer = layer.is_adjustment_layer;
    evaluated.local_time_seconds = timeline::local_time_from_composition(composition_time_seconds, layer.start_time);
    const std::uint64_t layer_seed = make_property_expression_seed(scene, composition, layer, "layer");
    const double remapped_time = sample_scalar(
        layer.time_remap_property,
        evaluated.local_time_seconds,
        evaluated.local_time_seconds,
        audio_analyzer,
        hash_combine(layer_seed, stable_string_hash("time_remap")),
        vars.numeric);
    evaluated.child_time_seconds = remapped_time;
    evaluated.active = layer.enabled && composition_time_seconds >= layer.in_point && composition_time_seconds <= layer.out_point;

    evaluated.opacity = sample_scalar(
        layer.opacity_property,
        layer.opacity,
        remapped_time,
        audio_analyzer,
        hash_combine(layer_seed, stable_string_hash("opacity")),
        vars.numeric);

    math::Vector2 pos = sample_vector2(
        layer.transform.position_property,
        fallback_position(layer),
        remapped_time,
        audio_analyzer,
        hash_combine(layer_seed, stable_string_hash("position")),
        vars.numeric);
    double rot = sample_scalar(
        layer.transform.rotation_property,
        fallback_rotation(layer),
        remapped_time,
        audio_analyzer,
        hash_combine(layer_seed, stable_string_hash("rotation")),
        vars.numeric);
    math::Vector2 scl = sample_vector2(
        layer.transform.scale_property,
        fallback_scale(layer),
        remapped_time,
        audio_analyzer,
        hash_combine(layer_seed, stable_string_hash("scale")),
        vars.numeric);
    
    evaluated.local_transform = make_transform2(pos, rot, scl);
    evaluated.world_matrix = evaluated.local_transform.to_matrix();
    evaluated.world_position3 = math::Vector3{pos.x, pos.y, 0.0f};

    if (layer.is_3d) {
        const math::Vector3 pos3 = sample_vector3(
            layer.transform3d.position_property,
            {pos.x, pos.y, 0.0f},
            remapped_time,
            audio_analyzer,
            hash_combine(layer_seed, stable_string_hash("position3")),
            vars.numeric);
        const math::Vector3 rot3 = sample_vector3(
            layer.transform3d.rotation_property,
            {0.0f, 0.0f, static_cast<float>(rot)},
            remapped_time,
            audio_analyzer,
            hash_combine(layer_seed, stable_string_hash("rotation3")),
            vars.numeric);
        const math::Vector3 scl3 = sample_vector3(
            layer.transform3d.scale_property,
            {scl.x, scl.y, 1.0f},
            remapped_time,
            audio_analyzer,
            hash_combine(layer_seed, stable_string_hash("scale3")),
            vars.numeric);
        
        math::Transform3 t3;
        t3.position = pos3;
        t3.rotation = math::Quaternion::from_euler({
            static_cast<float>(renderer2d::math_utils::degrees_to_radians(rot3.x)),
            static_cast<float>(renderer2d::math_utils::degrees_to_radians(rot3.y)),
            static_cast<float>(renderer2d::math_utils::degrees_to_radians(rot3.z))
        });
        t3.scale = scl3;
        evaluated.world_matrix = t3.to_matrix();
        evaluated.world_position3 = pos3;
    }

    evaluated.visible = evaluated.enabled && evaluated.active && evaluated.opacity > 0.0;
    evaluated.width = layer.width;
    evaluated.height = layer.height;
    evaluated.text_content = resolve_template(layer.text_content, vars.strings, vars.numeric);
    
    evaluated.fill_color = sample_color(layer.fill_color, {255, 255, 255, 255}, remapped_time);
    evaluated.stroke_color = sample_color(layer.stroke_color, {255, 255, 255, 255}, remapped_time);
    evaluated.stroke_width = static_cast<float>(sample_scalar(
        layer.stroke_width_property,
        static_cast<double>(layer.stroke_width),
        remapped_time,
        audio_analyzer,
        hash_combine(layer_seed, stable_string_hash("stroke_width")),
        vars.numeric));
    evaluated.line_cap = layer.line_cap;
    evaluated.line_join = layer.line_join;
    evaluated.miter_limit = layer.miter_limit;
    
    evaluated.effects = layer.effects;
    evaluated.subtitle_path = layer.subtitle_path;
    evaluated.subtitle_outline_color = layer.subtitle_outline_color;
    evaluated.subtitle_outline_width = layer.subtitle_outline_width;

    if (layer.shape_path.has_value()) {
        EvaluatedShapePath shape_path;
        shape_path.closed = layer.shape_path->closed;
        shape_path.points.reserve(layer.shape_path->points.size());
        for (const auto& point : layer.shape_path->points) {
            shape_path.points.push_back(EvaluatedShapePathPoint{
                point.position,
                point.tangent_in,
                point.tangent_out
            });
        }
        evaluated.shape_path = std::move(shape_path);
    }

    evaluated.track_matte_type = layer.track_matte_type;
    evaluated.precomp_id = layer.precomp_id;

    if ((evaluated.type == LayerType::Image || evaluated.type == LayerType::Video) && scene) {
        for (const auto& asset : scene->assets) {
            if (asset.id == evaluated.id || asset.id == evaluated.name) {
                evaluated.asset_id = asset.id;
                evaluated.asset_path = asset.path;
                break;
            }
        }
    }

    return evaluated;
}

struct EvaluationContext {
    const SceneSpec* scene{nullptr};
    const CompositionSpec& composition;
    std::int64_t frame_number{0};
    double composition_time_seconds{0.0};
    std::unordered_map<std::string, std::size_t> layer_indices;
    std::vector<std::optional<EvaluatedLayerState>> cache;
    std::vector<bool> visiting;
    std::vector<std::string> composition_stack;
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer{nullptr};
    EvaluationVariables vars;
    std::unordered_map<std::string, std::vector<text::SubtitleEntry>> subtitle_cache;
};

EvaluatedLightState evaluate_light_state(
    const EvaluatedLayerState& layer_state,
    const LayerSpec& spec,
    double remapped_time) {
    EvaluatedLightState light;
    light.layer_id = layer_state.id;
    light.type = spec.light_type.value_or("point");
    light.position = layer_state.world_position3;
    
    // Direction can be derived from world matrix (forward vector)
    const auto forward = layer_state.world_matrix.transform_vector({0.0f, 0.0f, -1.0f}).normalized();
    light.direction = forward;
    
    const std::uint64_t light_seed = hash_combine(stable_string_hash(layer_state.id), stable_string_hash(light.type));
    light.color = sample_color(spec.fill_color, {255, 255, 255, 255}, remapped_time);
    light.intensity = static_cast<float>(sample_scalar(spec.intensity, 1.0, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("intensity"))));
    light.attenuation_near = static_cast<float>(sample_scalar(spec.attenuation_near, 0.0, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("attenuation_near"))));
    light.attenuation_far = static_cast<float>(sample_scalar(spec.attenuation_far, 1000.0, remapped_time, nullptr, hash_combine(light_seed, stable_string_hash("attenuation_far"))));
    
    return light;
}

const EvaluatedLayerState& resolve_layer_state(
    std::size_t layer_index,
    EvaluationContext& context);

EvaluatedCompositionState evaluate_composition_internal(
    const SceneSpec* scene,
    const CompositionSpec& composition,
    std::int64_t frame_number,
    double composition_time_seconds,
    std::vector<std::string> stack,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars = {}) {
    
    EvaluatedCompositionState evaluated;
    evaluated.composition_id = composition.id;
    evaluated.composition_name = composition.name;
    evaluated.width = composition.width;
    evaluated.height = composition.height;
    evaluated.frame_rate = composition.frame_rate;
    evaluated.frame_number = frame_number;
    evaluated.composition_time_seconds = composition_time_seconds;
    evaluated.layers.reserve(composition.layers.size());

    EvaluationContext context{
        scene,
        composition,
        frame_number,
        composition_time_seconds,
        {},
        std::vector<std::optional<EvaluatedLayerState>>(composition.layers.size()),
        std::vector<bool>(composition.layers.size(), false),
        std::move(stack),
        audio_analyzer,
        vars
    };

    for (std::size_t index = 0; index < composition.layers.size(); ++index) {
        context.layer_indices.emplace(composition.layers[index].id, index);
    }

    for (std::size_t index = 0; index < composition.layers.size(); ++index) {
        const auto& layer_state = resolve_layer_state(index, context);
        if (layer_state.type == LayerType::Light) {
            evaluated.lights.push_back(evaluate_light_state(layer_state, composition.layers[index], layer_state.child_time_seconds));
        } else {
            evaluated.layers.push_back(layer_state);
        }
    }

    evaluated.camera = evaluate_camera_state(composition, evaluated.layers, frame_number, composition_time_seconds);
    return evaluated;
}

const EvaluatedLayerState& resolve_layer_state(
    std::size_t layer_index,
    EvaluationContext& context) {
    if (context.cache[layer_index].has_value()) {
        return *context.cache[layer_index];
    }

    if (context.visiting[layer_index]) {
        context.cache[layer_index] = make_layer_state(
            context.composition,
            context.composition.layers[layer_index],
            layer_index,
            context.frame_number,
            context.composition_time_seconds,
            context.scene,
            context.audio_analyzer,
            context.vars);
        return *context.cache[layer_index];
    }

    context.visiting[layer_index] = true;

    const auto& layer = context.composition.layers[layer_index];
    EvaluatedLayerState evaluated = make_layer_state(
        context.composition,
        layer,
        layer_index,
        context.frame_number,
        context.composition_time_seconds,
        context.scene,
        context.audio_analyzer,
        context.vars);

    // Resolve parent
    if (layer.parent.has_value() && !layer.parent->empty()) {
        const auto parent_it = context.layer_indices.find(*layer.parent);
        if (parent_it != context.layer_indices.end() && parent_it->second != layer_index) {
            const auto& parent = resolve_layer_state(parent_it->second, context);
            evaluated.world_matrix = parent.world_matrix * evaluated.world_matrix;
            const auto wp3 = evaluated.world_matrix.transform_point({0.0f, 0.0f, 0.0f});
            evaluated.world_position3 = wp3;
            evaluated.visible = evaluated.enabled && evaluated.active && parent.visible && evaluated.opacity > 0.0;
        }
    }

    // Resolve track matte
    if (layer.track_matte_layer_id.has_value() && !layer.track_matte_layer_id->empty()) {
        const auto matte_it = context.layer_indices.find(*layer.track_matte_layer_id);
        if (matte_it != context.layer_indices.end()) {
            evaluated.track_matte_layer_index = matte_it->second;
        }
    }

    // Resolve precomp
    if (evaluated.precomp_id.has_value() && !evaluated.precomp_id->empty() && context.scene) {
        bool circular = false;
        for (const auto& id : context.composition_stack) {
            if (id == *evaluated.precomp_id) {
                circular = true;
                break;
            }
        }

        if (!circular) {
            for (const auto& comp : context.scene->compositions) {
                if (comp.id == *evaluated.precomp_id) {
                    std::vector<std::string> next_stack = context.composition_stack;
                    next_stack.push_back(context.composition.id);
                    
                    const std::int64_t child_frame_number = static_cast<std::int64_t>(std::llround(
                        evaluated.child_time_seconds * 
                        static_cast<double>(comp.frame_rate.numerator) / 
                        static_cast<double>(comp.frame_rate.denominator)
                    ));

                    evaluated.nested_composition = std::make_unique<EvaluatedCompositionState>(
                        evaluate_composition_internal(context.scene, comp, child_frame_number, evaluated.child_time_seconds, std::move(next_stack), context.audio_analyzer, context.vars)
                    );
                    break;
                }
            }
        }
    }

    context.visiting[layer_index] = false;
    context.cache[layer_index] = std::move(evaluated);
    return *context.cache[layer_index];
}

} // namespace

EvaluatedLayerState evaluate_layer_state(
    const LayerSpec& layer,
    std::int64_t frame_number,
    double composition_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer) {
    CompositionSpec composition;
    composition.id = "standalone";
    composition.name = "standalone";
    return make_layer_state(composition, layer, 0, frame_number, composition_time_seconds, nullptr, audio_analyzer);
}

EvaluatedCameraState evaluate_camera_state(
    const CompositionSpec& composition,
    const std::vector<EvaluatedLayerState>& layers,
    std::int64_t frame_number,
    double composition_time_seconds
) {
    (void)frame_number;
    (void)composition_time_seconds;
    EvaluatedCameraState evaluated;
    evaluated.camera.aspect = composition.height > 0
        ? static_cast<float>(static_cast<double>(composition.width) / static_cast<double>(composition.height))
        : 1.0f;

    for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
        if (it->type != LayerType::Camera || !it->active) {
            continue;
        }

        evaluated.available = true;
        evaluated.layer_id = it->id;
        evaluated.position = it->world_position3;
        
        // Extract basic TRS from world matrix for simplicity in this pass
        // In real impl, we'd use decompose_matrix
        evaluated.camera.transform.compose_trs(
            evaluated.position,
            math::Quaternion::identity(), // Simplified for now
            math::Vector3::one()
        );
        break;
    }

    return evaluated;
}

EvaluatedCompositionState evaluate_composition_state(
    const CompositionSpec& composition,
    std::int64_t frame_number,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars) {
    const auto frame = timeline::sample_frame(composition.frame_rate, frame_number);
    return evaluate_composition_internal(nullptr, composition, frame.frame_number, frame.composition_time_seconds, {}, audio_analyzer, vars);
}

EvaluatedCompositionState evaluate_composition_state(
    const CompositionSpec& composition,
    double composition_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars) {
    const std::int64_t frame_number = static_cast<std::int64_t>(std::llround(composition_time_seconds * static_cast<double>(composition.frame_rate.numerator) / static_cast<double>(composition.frame_rate.denominator)));
    return evaluate_composition_internal(nullptr, composition, frame_number, composition_time_seconds, {}, audio_analyzer, vars);
}

std::optional<EvaluatedCompositionState> evaluate_scene_composition_state(
    const SceneSpec& scene,
    const std::string& composition_id,
    std::int64_t frame_number,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars
) {
    for (const auto& composition : scene.compositions) {
        if (composition.id == composition_id) {
            const auto frame = timeline::sample_frame(composition.frame_rate, frame_number);
            return evaluate_composition_internal(&scene, composition, frame.frame_number, frame.composition_time_seconds, {}, audio_analyzer, vars);
        }
    }
    return std::nullopt;
}

std::optional<EvaluatedCompositionState> evaluate_scene_composition_state(
    const SceneSpec& scene,
    const std::string& composition_id,
    double composition_time_seconds,
    const ::tachyon::audio::AudioAnalyzer* audio_analyzer,
    EvaluationVariables vars
) {
    for (const auto& composition : scene.compositions) {
        if (composition.id == composition_id) {
            const std::int64_t frame_number = static_cast<std::int64_t>(std::llround(composition_time_seconds * static_cast<double>(composition.frame_rate.numerator) / static_cast<double>(composition.frame_rate.denominator)));
            return evaluate_composition_internal(&scene, composition, frame_number, composition_time_seconds, {}, audio_analyzer, vars);
        }
    }
    return std::nullopt;
}

EvaluatedLayerState::EvaluatedLayerState(const EvaluatedLayerState& other) {
    layer_index = other.layer_index;
    id = other.id;
    name = other.name;
    type = other.type;
    blend_mode = other.blend_mode;
    enabled = other.enabled;
    active = other.active;
    visible = other.visible;
    is_3d = other.is_3d;
    is_adjustment_layer = other.is_adjustment_layer;
    local_time_seconds = other.local_time_seconds;
    child_time_seconds = other.child_time_seconds;
    opacity = other.opacity;
    local_transform = other.local_transform;
    world_matrix = other.world_matrix;
    world_position3 = other.world_position3;
    width = other.width;
    height = other.height;
    text_content = other.text_content;
    fill_color = other.fill_color;
    stroke_color = other.stroke_color;
    stroke_width = other.stroke_width;
    line_cap = other.line_cap;
    line_join = other.line_join;
    miter_limit = other.miter_limit;
    shape_path = other.shape_path;
    effects = other.effects;
    track_matte_type = other.track_matte_type;
    track_matte_layer_index = other.track_matte_layer_index;
    precomp_id = other.precomp_id;
    if (other.nested_composition) {
        nested_composition = std::make_unique<EvaluatedCompositionState>(*other.nested_composition);
    }
    asset_id = other.asset_id;
    asset_path = other.asset_path;
}

EvaluatedLayerState& EvaluatedLayerState::operator=(const EvaluatedLayerState& other) {
    if (this == &other) return *this;
    layer_index = other.layer_index;
    id = other.id;
    name = other.name;
    type = other.type;
    blend_mode = other.blend_mode;
    enabled = other.enabled;
    active = other.active;
    visible = other.visible;
    is_3d = other.is_3d;
    is_adjustment_layer = other.is_adjustment_layer;
    local_time_seconds = other.local_time_seconds;
    child_time_seconds = other.child_time_seconds;
    opacity = other.opacity;
    local_transform = other.local_transform;
    world_matrix = other.world_matrix;
    world_position3 = other.world_position3;
    width = other.width;
    height = other.height;
    text_content = other.text_content;
    fill_color = other.fill_color;
    stroke_color = other.stroke_color;
    stroke_width = other.stroke_width;
    line_cap = other.line_cap;
    line_join = other.line_join;
    miter_limit = other.miter_limit;
    shape_path = other.shape_path;
    effects = other.effects;
    track_matte_type = other.track_matte_type;
    track_matte_layer_index = other.track_matte_layer_index;
    precomp_id = other.precomp_id;
    if (other.nested_composition) {
        nested_composition = std::make_unique<EvaluatedCompositionState>(*other.nested_composition);
    } else {
        nested_composition.reset();
    }
    asset_id = other.asset_id;
    asset_path = other.asset_path;
    return *this;
}

} // namespace scene
} // namespace tachyon

