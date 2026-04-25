#include "tachyon/core/spec/schema/objects/scene_spec_core.h"

namespace tachyon {

template <typename KeyframeT>
bool parse_keyframe_common(const json& object, KeyframeT& keyframe, const std::string& path, DiagnosticBag& diagnostics) {
    if (!object.contains("time") || !object.at("time").is_number()) {
        diagnostics.add_error("scene.layer.keyframe.time_invalid", "keyframe.time must be numeric", path + ".time");
        return false;
    }
    keyframe.time = object.at("time").get<double>();
    if (object.contains("easing")) {
        keyframe.easing = parse_easing_preset(object.at("easing"));
        if (keyframe.easing == animation::EasingPreset::Custom) {
            if (object.contains("bezier")) {
                keyframe.bezier = parse_bezier(object.at("bezier"));
            } else {
                if (object.contains("speed_in")) keyframe.speed_in = object.at("speed_in").get<double>();
                if (object.contains("influence_in")) keyframe.influence_in = object.at("influence_in").get<double>();
                if (object.contains("speed_out")) keyframe.speed_out = object.at("speed_out").get<double>();
                if (object.contains("influence_out")) keyframe.influence_out = object.at("influence_out").get<double>();
            }
        }
    }
    return true;
}

bool parse_scalar_keyframe(const json& object, ScalarKeyframeSpec& keyframe, const std::string& path, DiagnosticBag& diagnostics) {
    if (!parse_keyframe_common(object, keyframe, path, diagnostics)) return false;
    if (!object.contains("value") || !object.at("value").is_number()) {
        diagnostics.add_error("scene.layer.keyframe.value_invalid", "scalar keyframe value must be numeric", path + ".value");
        return false;
    }
    keyframe.value = object.at("value").get<double>();
    return true;
}

bool parse_vector2_keyframe(const json& object, Vector2KeyframeSpec& keyframe, const std::string& path, DiagnosticBag& diagnostics) {
    if (!parse_keyframe_common(object, keyframe, path, diagnostics)) return false;
    if (!object.contains("value") || !parse_vector2_value(object.at("value"), keyframe.value)) {
        diagnostics.add_error("scene.layer.keyframe.value_invalid", "vector2 keyframe value must be array or object", path + ".value");
        return false;
    }
    if (object.contains("spatial_in")) parse_vector2_value(object.at("spatial_in"), keyframe.tangent_in);
    if (object.contains("spatial_out")) parse_vector2_value(object.at("spatial_out"), keyframe.tangent_out);
    return true;
}

bool parse_vector3_keyframe(const json& object, AnimatedVector3Spec::Keyframe& keyframe, const std::string& path, DiagnosticBag& diagnostics) {
    if (!parse_keyframe_common(object, keyframe, path, diagnostics)) return false;
    if (!object.contains("value") || !parse_vector3_value(object.at("value"), keyframe.value)) {
        diagnostics.add_error("scene.layer.keyframe.value_invalid", "vector3 keyframe value must be array or object", path + ".value");
        return false;
    }
    if (object.contains("spatial_in")) parse_vector3_value(object.at("spatial_in"), keyframe.tangent_in);
    if (object.contains("spatial_out")) parse_vector3_value(object.at("spatial_out"), keyframe.tangent_out);
    return true;
}

bool parse_color_keyframe(const json& object, ColorKeyframeSpec& keyframe, const std::string& path, DiagnosticBag& diagnostics) {
    if (!parse_keyframe_common(object, keyframe, path, diagnostics)) return false;
    if (!object.contains("value") || !parse_color_value(object.at("value"), keyframe.value)) {
        diagnostics.add_error("scene.layer.keyframe.value_invalid", "color keyframe value must be array or object", path + ".value");
        return false;
    }
    return true;
}

void parse_optional_scalar_property(const json& object, const char* key, AnimatedScalarSpec& property, const std::string& path, DiagnosticBag& diagnostics) {
    if (!object.contains(key) || object.at(key).is_null()) return;
    const auto& value = object.at(key);
    if (value.is_number()) { property.value = value.get<double>(); return; }
    if (value.is_object()) {
        if (value.contains("value") && value.at("value").is_number()) property.value = value.at("value").get<double>();
        if (value.contains("audio_band")) {
            const auto audio_band = parse_audio_band_type(value.at("audio_band"));
            if (audio_band.has_value()) {
                property.audio_band = *audio_band;
                if (value.contains("min") && value.at("min").is_number()) property.audio_min = value.at("min").get<double>();
                if (value.contains("max") && value.at("max").is_number()) property.audio_max = value.at("max").get<double>();
                return;
            }
            diagnostics.add_error("scene.layer.property_invalid", std::string(key) + ".audio_band invalid", make_path(path, key));
            return;
        }
    if (value.contains("keyframes") && value.at("keyframes").is_array()) {
        for (std::size_t i = 0; i < value.at("keyframes").size(); ++i) {
            ScalarKeyframeSpec k;
            if (parse_scalar_keyframe(value.at("keyframes")[i], k, make_path(path, key) + ".keyframes[" + std::to_string(i) + "]", diagnostics)) property.keyframes.push_back(k);
        }
        // Post-process: generate beziers from AE speed/influence for each keyframe
        for (std::size_t i = 0; i + 1 < property.keyframes.size(); ++i) {
            auto& kf = property.keyframes[i];
            const auto& next_kf = property.keyframes[i + 1];
            const double duration = next_kf.time - kf.time;
            const double value_delta = next_kf.value - kf.value;
            if (duration <= 0.0 || std::abs(value_delta) < 1e-8) continue;
            // If speed_out/influence_out are set, generate bezier from AE params
            if (kf.speed_out != 0.0 || kf.influence_out != 33.333333333) {
                kf.bezier = animation::CubicBezierEasing::from_ae(
                    kf.speed_out, kf.influence_out,
                    kf.speed_in, kf.influence_in,
                    duration, value_delta
                );
                kf.easing = animation::EasingPreset::Custom;
            }
        }
        return;
    }
        if (value.contains("expression") && value.at("expression").is_string()) { property.expression = value.at("expression").get<std::string>(); return; }
    }
}

void parse_optional_vector_property(const json& object, const char* key, AnimatedVector2Spec& property, const std::string& path, DiagnosticBag& diagnostics) {
    if (!object.contains(key) || object.at(key).is_null()) return;
    const auto& value = object.at(key);
    math::Vector2 v;
    if (parse_vector2_value(value, v)) { property.value = v; return; }
    if (value.is_object()) {
        if (value.contains("value") && parse_vector2_value(value.at("value"), v)) property.value = v;
    if (value.contains("keyframes") && value.at("keyframes").is_array()) {
        for (std::size_t i = 0; i < value.at("keyframes").size(); ++i) {
            Vector2KeyframeSpec k;
            if (parse_vector2_keyframe(value.at("keyframes")[i], k, make_path(path, key) + ".keyframes[" + std::to_string(i) + "]", diagnostics)) property.keyframes.push_back(k);
        }
        // Post-process Vector2 keyframes for AE speed/influence
        for (std::size_t i = 0; i + 1 < property.keyframes.size(); ++i) {
            auto& kf = property.keyframes[i];
            const auto& next_kf = property.keyframes[i + 1];
            const double duration = next_kf.time - kf.time;
            const math::Vector2 value_delta = next_kf.value - kf.value;
            const double delta_mag = std::sqrt(value_delta.x * value_delta.x + value_delta.y * value_delta.y);
            if (duration <= 0.0 || delta_mag < 1e-8) continue;
            if (kf.speed_out != 0.0 || kf.influence_out != 33.333333333) {
                kf.bezier = animation::CubicBezierEasing::from_ae(
                    kf.speed_out, kf.influence_out,
                    kf.speed_in, kf.influence_in,
                    duration, delta_mag
                );
                kf.easing = animation::EasingPreset::Custom;
            }
        }
        return;
    }
        if (value.contains("expression") && value.at("expression").is_string()) { property.expression = value.at("expression").get<std::string>(); return; }
    }
}

void parse_optional_vector3_property(const json& object, const char* key, AnimatedVector3Spec& property, const std::string& path, DiagnosticBag& diagnostics) {
    if (!object.contains(key) || object.at(key).is_null()) return;
    const auto& value = object.at(key);
    math::Vector3 v;
    if (parse_vector3_value(value, v)) { property.value = v; return; }
    if (value.is_object()) {
        if (value.contains("value") && parse_vector3_value(value.at("value"), v)) property.value = v;
    if (value.contains("keyframes") && value.at("keyframes").is_array()) {
        for (std::size_t i = 0; i < value.at("keyframes").size(); ++i) {
            AnimatedVector3Spec::Keyframe k;
            if (parse_vector3_keyframe(value.at("keyframes")[i], k, make_path(path, key) + ".keyframes[" + std::to_string(i) + "]", diagnostics)) property.keyframes.push_back(k);
        }
        // Post-process Vector3 keyframes for AE speed/influence
        for (std::size_t i = 0; i + 1 < property.keyframes.size(); ++i) {
            auto& kf = property.keyframes[i];
            const auto& next_kf = property.keyframes[i + 1];
            const double duration = next_kf.time - kf.time;
            const math::Vector3 value_delta = next_kf.value - kf.value;
            const double delta_mag = std::sqrt(value_delta.x * value_delta.x + value_delta.y * value_delta.y + value_delta.z * value_delta.z);
            if (duration <= 0.0 || delta_mag < 1e-8) continue;
            if (kf.speed_out != 0.0 || kf.influence_out != 33.333333333) {
                kf.bezier = animation::CubicBezierEasing::from_ae(
                    kf.speed_out, kf.influence_out,
                    kf.speed_in, kf.influence_in,
                    duration, delta_mag
                );
                kf.easing = animation::EasingPreset::Custom;
            }
        }
        return;
    }
        if (value.contains("expression") && value.at("expression").is_string()) { property.expression = value.at("expression").get<std::string>(); return; }
    }
}

void parse_optional_color_property(const json& object, const char* key, AnimatedColorSpec& property, const std::string& path, DiagnosticBag& diagnostics) {
    if (!object.contains(key) || object.at(key).is_null()) return;
    const auto& value = object.at(key);
    ColorSpec c;
    if (parse_color_value(value, c)) { property.value = c; return; }
    if (value.is_object()) {
        if (value.contains("value") && parse_color_value(value.at("value"), c)) property.value = c;
    if (value.contains("keyframes") && value.at("keyframes").is_array()) {
        for (std::size_t i = 0; i < value.at("keyframes").size(); ++i) {
            ColorKeyframeSpec k;
            if (parse_color_keyframe(value.at("keyframes")[i], k, make_path(path, key) + ".keyframes[" + std::to_string(i) + "]", diagnostics)) property.keyframes.push_back(k);
        }
        // Post-process Color keyframes for AE speed/influence
        for (std::size_t i = 0; i + 1 < property.keyframes.size(); ++i) {
            auto& kf = property.keyframes[i];
            const auto& next_kf = property.keyframes[i + 1];
            const double duration = next_kf.time - kf.time;
            // Value delta as max channel difference (simplified for color)
            const double value_delta = std::max({
                std::abs(next_kf.value.r - kf.value.r),
                std::abs(next_kf.value.g - kf.value.g),
                std::abs(next_kf.value.b - kf.value.b),
                std::abs(next_kf.value.a - kf.value.a)
            }) / 255.0;
            if (duration <= 0.0 || value_delta < 1e-8) continue;
            if (kf.speed_out != 0.0 || kf.influence_out != 33.333333333) {
                kf.bezier = animation::CubicBezierEasing::from_ae(
                    kf.speed_out, kf.influence_out,
                    kf.speed_in, kf.influence_in,
                    duration, value_delta
                );
                kf.easing = animation::EasingPreset::Custom;
            }
        }
    }
    }
}

} // namespace tachyon
