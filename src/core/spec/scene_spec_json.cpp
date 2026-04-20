#include "tachyon/core/spec/scene_spec_core.h"
#include <algorithm>
#include <sstream>

namespace tachyon {

bool read_string(const json& object, const char* key, std::string& out) {
    if (!object.contains(key) || object.at(key).is_null()) return false;
    const auto& value = object.at(key);
    if (!value.is_string()) return false;
    out = value.get<std::string>();
    return true;
}

bool read_bool(const json& object, const char* key, bool& out) {
    if (!object.contains(key) || object.at(key).is_null()) return false;
    const auto& value = object.at(key);
    if (!value.is_boolean()) return false;
    out = value.get<bool>();
    return true;
}

bool read_optional_int(const json& object, const char* key, std::optional<std::int64_t>& out) {
    if (!object.contains(key) || object.at(key).is_null()) return false;
    const auto& value = object.at(key);
    if (!value.is_number_integer()) return false;
    out = value.get<std::int64_t>();
    return true;
}

bool read_vector2_like(const json& value, math::Vector2& out) {
    if (value.is_array()) {
        if (value.size() < 2 || !value[0].is_number() || !value[1].is_number()) return false;
        out = { static_cast<float>(value[0].get<double>()), static_cast<float>(value[1].get<double>()) };
        return true;
    }
    if (value.is_number()) {
        const float scalar = static_cast<float>(value.get<double>());
        out = {scalar, scalar};
        return true;
    }
    return false;
}

bool read_vector2_object(const json& value, math::Vector2& out) {
    if (!value.is_object()) return false;
    if (!value.contains("x") || !value.contains("y") || !value.at("x").is_number() || !value.at("y").is_number()) return false;
    out = { static_cast<float>(value.at("x").get<double>()), static_cast<float>(value.at("y").get<double>()) };
    return true;
}

bool parse_vector2_value(const json& value, math::Vector2& out) {
    return read_vector2_like(value, out) || read_vector2_object(value, out);
}

bool read_vector3_like(const json& value, math::Vector3& out) {
    if (value.is_array()) {
        if (value.size() < 3 || !value[0].is_number() || !value[1].is_number() || !value[2].is_number()) return false;
        out = { static_cast<float>(value[0].get<double>()), static_cast<float>(value[1].get<double>()), static_cast<float>(value[2].get<double>()) };
        return true;
    }
    if (value.is_number()) {
        const float s = static_cast<float>(value.get<double>());
        out = {s, s, s};
        return true;
    }
    return false;
}

bool read_vector3_object(const json& value, math::Vector3& out) {
    if (!value.is_object()) return false;
    if (!value.contains("x") || !value.contains("y") || !value.contains("z") ||
        !value.at("x").is_number() || !value.at("y").is_number() || !value.at("z").is_number()) return false;
    out = { static_cast<float>(value.at("x").get<double>()), static_cast<float>(value.at("y").get<double>()), static_cast<float>(value.at("z").get<double>()) };
    return true;
}

bool parse_vector3_value(const json& value, math::Vector3& out) {
    return read_vector3_like(value, out) || read_vector3_object(value, out);
}

bool parse_color_value(const json& value, ColorSpec& out) {
    if (value.is_array()) {
        if (value.size() < 3) return false;
        out.r = static_cast<std::uint8_t>(std::clamp(value[0].get<int>(), 0, 255));
        out.g = static_cast<std::uint8_t>(std::clamp(value[1].get<int>(), 0, 255));
        out.b = static_cast<std::uint8_t>(std::clamp(value[2].get<int>(), 0, 255));
        out.a = (value.size() >= 4) ? static_cast<std::uint8_t>(std::clamp(value[3].get<int>(), 0, 255)) : 255;
        return true;
    }
    if (value.is_object()) {
        if (value.contains("r") && value.contains("g") && value.contains("b")) {
            out.r = static_cast<std::uint8_t>(std::clamp(value.at("r").get<int>(), 0, 255));
            out.g = static_cast<std::uint8_t>(std::clamp(value.at("g").get<int>(), 0, 255));
            out.b = static_cast<std::uint8_t>(std::clamp(value.at("b").get<int>(), 0, 255));
            out.a = (value.contains("a")) ? static_cast<std::uint8_t>(std::clamp(value.at("a").get<int>(), 0, 255)) : 255;
            return true;
        }
    }
    return false;
}

animation::EasingPreset parse_easing_preset(const json& value) {
    if (!value.is_string()) return animation::EasingPreset::None;
    const std::string easing = value.get<std::string>();
    if (easing == "ease_in" || easing == "easeIn") return animation::EasingPreset::EaseIn;
    if (easing == "ease_out" || easing == "easeOut") return animation::EasingPreset::EaseOut;
    if (easing == "ease_in_out" || easing == "easeInOut") return animation::EasingPreset::EaseInOut;
    if (easing == "custom" || easing == "bezier") return animation::EasingPreset::Custom;
    return animation::EasingPreset::None;
}

animation::CubicBezierEasing parse_bezier(const json& value) {
    animation::CubicBezierEasing bezier = animation::CubicBezierEasing::linear();
    if (!value.is_object()) return bezier;
    if (value.contains("cx1") && value.at("cx1").is_number()) bezier.cx1 = value.at("cx1").get<double>();
    if (value.contains("cy1") && value.at("cy1").is_number()) bezier.cy1 = value.at("cy1").get<double>();
    if (value.contains("cx2") && value.at("cx2").is_number()) bezier.cx2 = value.at("cx2").get<double>();
    if (value.contains("cy2") && value.at("cy2").is_number()) bezier.cy2 = value.at("cy2").get<double>();
    return bezier;
}

renderer2d::LineCap parse_line_cap(const json& value) {
    if (!value.is_string()) return renderer2d::LineCap::Butt;
    std::string s = value.get<std::string>();
    if (s == "round") return renderer2d::LineCap::Round;
    if (s == "square") return renderer2d::LineCap::Square;
    return renderer2d::LineCap::Butt;
}

renderer2d::LineJoin parse_line_join(const json& value) {
    if (!value.is_string()) return renderer2d::LineJoin::Miter;
    std::string s = value.get<std::string>();
    if (s == "round") return renderer2d::LineJoin::Round;
    if (s == "bevel") return renderer2d::LineJoin::Bevel;
    return renderer2d::LineJoin::Miter;
}

std::optional<AudioBandType> parse_audio_band_type(const json& value) {
    if (!value.is_string()) return std::nullopt;
    const std::string band = value.get<std::string>();
    if (band == "bass") return AudioBandType::Bass;
    if (band == "mid") return AudioBandType::Mid;
    if (band == "high") return AudioBandType::High;
    if (band == "presence") return AudioBandType::Presence;
    if (band == "rms") return AudioBandType::Rms;
    return std::nullopt;
}

template <typename KeyframeT>
bool parse_keyframe_common(const json& object, KeyframeT& keyframe, const std::string& path, DiagnosticBag& diagnostics) {
    if (!object.contains("time") || !object.at("time").is_number()) {
        diagnostics.add_error("scene.layer.keyframe.time_invalid", "keyframe.time must be numeric", path + ".time");
        return false;
    }
    keyframe.time = object.at("time").get<double>();
    if (object.contains("easing")) {
        keyframe.easing = parse_easing_preset(object.at("easing"));
        if (keyframe.easing == animation::EasingPreset::Custom && object.contains("bezier")) {
            keyframe.bezier = parse_bezier(object.at("bezier"));
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
        }
    }
}

void parse_transform(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    if (object.contains("position") && !object.at("position").is_null()) {
        math::Vector2 p;
        if (parse_vector2_value(object.at("position"), p)) {
            layer.transform.position_x = static_cast<double>(p.x);
            layer.transform.position_y = static_cast<double>(p.y);
        }
    }
    if (object.contains("rotation") && !object.at("rotation").is_null()) {
        if (object.at("rotation").is_number()) layer.transform.rotation = object.at("rotation").get<double>();
    }
    if (object.contains("scale") && !object.at("scale").is_null()) {
        math::Vector2 s;
        if (parse_vector2_value(object.at("scale"), s)) {
            layer.transform.scale_x = static_cast<double>(s.x);
            layer.transform.scale_y = static_cast<double>(s.y);
        }
    }
    parse_optional_vector_property(object, "position_property", layer.transform.position_property, path, diagnostics);
    parse_optional_scalar_property(object, "rotation_property", layer.transform.rotation_property, path, diagnostics);
    parse_optional_vector_property(object, "scale_property", layer.transform.scale_property, path, diagnostics);
    
    parse_optional_vector3_property(object, "position3_property", layer.transform3d.position_property, path, diagnostics);
    parse_optional_vector3_property(object, "orientation_property", layer.transform3d.orientation_property, path, diagnostics);
    parse_optional_vector3_property(object, "rotation3_property", layer.transform3d.rotation_property, path, diagnostics);
    parse_optional_vector3_property(object, "scale3_property", layer.transform3d.scale_property, path, diagnostics);
    parse_optional_vector3_property(object, "anchor_point3_property", layer.transform3d.anchor_point_property, path, diagnostics);
}

void parse_shape_path(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    (void)path; (void)diagnostics;
    if (!object.contains("shape_path") || object.at("shape_path").is_null()) return;
    const auto& shape_path = object.at("shape_path");
    if (!shape_path.is_object() || !shape_path.contains("points") || !shape_path.at("points").is_array()) return;
    ShapePathSpec parsed;
    if (shape_path.contains("closed") && shape_path.at("closed").is_boolean()) parsed.closed = shape_path.at("closed").get<bool>();
    const auto& points = shape_path.at("points");
    for (std::size_t i = 0; i < points.size(); ++i) {
        ShapePathPointSpec p;
        if (parse_vector2_value(points[i], p.position)) {
            if (points[i].is_object()) {
                if (points[i].contains("tangent_in")) parse_vector2_value(points[i].at("tangent_in"), p.tangent_in);
                if (points[i].contains("tangent_out")) parse_vector2_value(points[i].at("tangent_out"), p.tangent_out);
            }
            parsed.points.push_back(p);
        }
    }
    if (!parsed.points.empty()) layer.shape_path = std::move(parsed);
}

LayerSpec parse_layer(const json& object, const std::string& path, DiagnosticBag& diagnostics) {
    LayerSpec layer;
    read_string(object, "id", layer.id);
    read_string(object, "type", layer.type);
    read_string(object, "name", layer.name);
    read_string(object, "blend_mode", layer.blend_mode);
    read_bool(object, "enabled", layer.enabled);
    read_bool(object, "visible", layer.visible);
    read_bool(object, "is_3d", layer.is_3d);
    read_number(object, "start_time", layer.start_time);
    read_number(object, "in_point", layer.in_point);
    read_number(object, "out_point", layer.out_point);
    read_number(object, "opacity", layer.opacity);
    if (object.contains("parent") && object.at("parent").is_string()) layer.parent = object.at("parent").get<std::string>();
    if (object.contains("transform") && object.at("transform").is_object()) parse_transform(object.at("transform"), layer, path, diagnostics);
    parse_optional_scalar_property(object, "opacity_property", layer.opacity_property, path, diagnostics);
    parse_shape_path(object, layer, path, diagnostics);
    parse_optional_scalar_property(object, "time_remap", layer.time_remap_property, path, diagnostics);
    if (object.contains("text_content")) layer.text_content = object.at("text_content").get<std::string>();
    if (object.contains("font_id")) layer.font_id = object.at("font_id").get<std::string>();
    parse_optional_scalar_property(object, "font_size", layer.font_size, path, diagnostics);
    if (object.contains("alignment")) layer.alignment = object.at("alignment").get<std::string>();
    if (object.contains("precomp_id")) layer.precomp_id = object.at("precomp_id").get<std::string>();
    if (object.contains("mesh_path")) layer.mesh_path = object.at("mesh_path").get<std::string>();
    
    // Subtitle burn-in (for text layers)
    if (object.contains("subtitle_path")) layer.subtitle_path = object.at("subtitle_path").get<std::string>();
    if (object.contains("subtitle_outline_width")) layer.subtitle_outline_width = object.at("subtitle_outline_width").get<float>();
    if (object.contains("subtitle_outline_color")) {
        const auto& color_val = object.at("subtitle_outline_color");
        if (color_val.is_array() && color_val.size() >= 3) {
            ColorSpec cs;
            cs.r = static_cast<std::uint8_t>(std::clamp(color_val[0].get<int>(), 0, 255));
            cs.g = static_cast<std::uint8_t>(std::clamp(color_val[1].get<int>(), 0, 255));
            cs.b = static_cast<std::uint8_t>(std::clamp(color_val[2].get<int>(), 0, 255));
            cs.a = (color_val.size() >= 4) ? static_cast<std::uint8_t>(std::clamp(color_val[3].get<int>(), 0, 255)) : 255;
            layer.subtitle_outline_color = cs;
        }
    }
    
    if (object.contains("stroke_width")) layer.stroke_width = object.at("stroke_width").get<float>();
    parse_optional_scalar_property(object, "stroke_width_property", layer.stroke_width_property, path, diagnostics);
    if (object.contains("line_cap")) layer.line_cap = parse_line_cap(object.at("line_cap"));
    if (object.contains("line_join")) layer.line_join = parse_line_join(object.at("line_join"));
    if (object.contains("miter_limit")) layer.miter_limit = object.at("miter_limit").get<float>();
    parse_optional_color_property(object, "fill_color", layer.fill_color, path, diagnostics);
    parse_optional_color_property(object, "stroke_color", layer.stroke_color, path, diagnostics);
    if (object.contains("light_type")) layer.light_type = object.at("light_type").get<std::string>();
    parse_optional_scalar_property(object, "intensity", layer.intensity, path, diagnostics);
    parse_optional_scalar_property(object, "attenuation_near", layer.attenuation_near, path, diagnostics);
    parse_optional_scalar_property(object, "attenuation_far", layer.attenuation_far, path, diagnostics);
    parse_optional_scalar_property(object, "cone_angle", layer.cone_angle, path, diagnostics);
    parse_optional_scalar_property(object, "cone_feather", layer.cone_feather, path, diagnostics);
    read_string(object, "falloff_type", layer.falloff_type);
    read_bool(object, "casts_shadows", layer.casts_shadows);
    parse_optional_scalar_property(object, "shadow_darkness", layer.shadow_darkness, path, diagnostics);
    parse_optional_scalar_property(object, "shadow_radius", layer.shadow_radius, path, diagnostics);

    // Material options
    parse_optional_scalar_property(object, "ambient_coeff", layer.ambient_coeff, path, diagnostics);
    parse_optional_scalar_property(object, "diffuse_coeff", layer.diffuse_coeff, path, diagnostics);
    parse_optional_scalar_property(object, "specular_coeff", layer.specular_coeff, path, diagnostics);
    parse_optional_scalar_property(object, "shininess", layer.shininess, path, diagnostics);
    parse_optional_scalar_property(object, "metallic", layer.metallic, path, diagnostics);
    parse_optional_scalar_property(object, "roughness", layer.roughness, path, diagnostics);
    parse_optional_scalar_property(object, "emission", layer.emission, path, diagnostics);
    parse_optional_scalar_property(object, "ior", layer.ior, path, diagnostics);
    read_bool(object, "metal", layer.metal);

    // Extrusion
    parse_optional_scalar_property(object, "extrusion_depth", layer.extrusion_depth, path, diagnostics);
    parse_optional_scalar_property(object, "bevel_size", layer.bevel_size, path, diagnostics);

    // Camera
    read_bool(object, "is_two_node", layer.is_two_node);
    parse_optional_vector3_property(object, "point_of_interest", layer.point_of_interest, path, diagnostics);
    read_bool(object, "dof_enabled", layer.dof_enabled);
    parse_optional_scalar_property(object, "focus_distance", layer.focus_distance, path, diagnostics);
    parse_optional_scalar_property(object, "aperture", layer.aperture, path, diagnostics);
    parse_optional_scalar_property(object, "blur_level", layer.blur_level, path, diagnostics);
    parse_optional_scalar_property(object, "aperture_blades", layer.aperture_blades, path, diagnostics);
    parse_optional_scalar_property(object, "aperture_rotation", layer.aperture_rotation, path, diagnostics);

    // Parse text_animators array (for text layers)
    if (object.contains("animators") && object.at("animators").is_array()) {
        const auto& anim_array = object.at("animators");
        for (std::size_t ai = 0; ai < anim_array.size(); ++ai) {
            const auto& anim_obj = anim_array[ai];
            if (!anim_obj.is_object()) continue;

            TextAnimatorSpec anim;

            // Selector
            if (anim_obj.contains("selector") && anim_obj.at("selector").is_object()) {
                const auto& sel = anim_obj.at("selector");
                if (sel.contains("type") && sel.at("type").is_string())
                    anim.selector.type = sel.at("type").get<std::string>();
                if (sel.contains("start") && sel.at("start").is_number())
                    anim.selector.start = sel.at("start").get<double>();
                if (sel.contains("end") && sel.at("end").is_number())
                    anim.selector.end = sel.at("end").get<double>();
            }

            // Opacity
            if (anim_obj.contains("opacity")) {
                const auto& op = anim_obj.at("opacity");
                if (op.is_number()) {
                    anim.properties.opacity_value = op.get<double>();
                } else if (op.is_object() && op.contains("keyframes") && op.at("keyframes").is_array()) {
                    const auto& kfs = op.at("keyframes");
                    for (std::size_t ki = 0; ki < kfs.size(); ++ki) {
                        ScalarKeyframeSpec k;
                        if (parse_scalar_keyframe(kfs[ki], k, path + ".animators[" + std::to_string(ai) + "].opacity.keyframes[" + std::to_string(ki) + "]", diagnostics))
                            anim.properties.opacity_keyframes.push_back(k);
                    }
                }
            }

            // Position offset
            if (anim_obj.contains("position")) {
                const auto& pos = anim_obj.at("position");
                math::Vector2 pv;
                if (parse_vector2_value(pos, pv)) {
                    anim.properties.position_offset_value = pv;
                } else if (pos.is_object() && pos.contains("keyframes") && pos.at("keyframes").is_array()) {
                    const auto& kfs = pos.at("keyframes");
                    for (std::size_t ki = 0; ki < kfs.size(); ++ki) {
                        Vector2KeyframeSpec k;
                        if (parse_vector2_keyframe(kfs[ki], k, path + ".animators[" + std::to_string(ai) + "].position.keyframes[" + std::to_string(ki) + "]", diagnostics))
                            anim.properties.position_offset_keyframes.push_back(k);
                    }
                }
            }

            // Scale
            if (anim_obj.contains("scale")) {
                const auto& sc = anim_obj.at("scale");
                if (sc.is_number()) {
                    anim.properties.scale_value = sc.get<double>();
                } else if (sc.is_object() && sc.contains("keyframes") && sc.at("keyframes").is_array()) {
                    const auto& kfs = sc.at("keyframes");
                    for (std::size_t ki = 0; ki < kfs.size(); ++ki) {
                        ScalarKeyframeSpec k;
                        if (parse_scalar_keyframe(kfs[ki], k, path + ".animators[" + std::to_string(ai) + "].scale.keyframes[" + std::to_string(ki) + "]", diagnostics))
                            anim.properties.scale_keyframes.push_back(k);
                    }
                }
            }

            // Rotation
            if (anim_obj.contains("rotation")) {
                const auto& rot = anim_obj.at("rotation");
                if (rot.is_number()) {
                    anim.properties.rotation_value = rot.get<double>();
                } else if (rot.is_object() && rot.contains("keyframes") && rot.at("keyframes").is_array()) {
                    const auto& kfs = rot.at("keyframes");
                    for (std::size_t ki = 0; ki < kfs.size(); ++ki) {
                        ScalarKeyframeSpec k;
                        if (parse_scalar_keyframe(kfs[ki], k, path + ".animators[" + std::to_string(ai) + "].rotation.keyframes[" + std::to_string(ki) + "]", diagnostics))
                            anim.properties.rotation_keyframes.push_back(k);
                    }
                }
            }

            layer.text_animators.push_back(std::move(anim));
        }
    }

    return layer;
}


CompositionSpec parse_composition(const json& object, const std::string& path, DiagnosticBag& diagnostics) {
    CompositionSpec composition;
    read_string(object, "id", composition.id);
    read_string(object, "name", composition.name);
    read_number(object, "width", composition.width);
    read_number(object, "height", composition.height);
    read_number(object, "duration", composition.duration);
    if (object.contains("fps") && object.at("fps").is_number_integer()) {
        composition.fps = object.at("fps").get<std::int64_t>();
        composition.frame_rate.numerator = *composition.fps;
        composition.frame_rate.denominator = 1;
    }
    if (object.contains("frame_rate")) {
        const auto& fr = object.at("frame_rate");
        if (fr.is_number_integer()) { composition.frame_rate.numerator = fr.get<std::int64_t>(); composition.frame_rate.denominator = 1; }
        else if (fr.is_object()) { read_number(fr, "numerator", composition.frame_rate.numerator); read_number(fr, "denominator", composition.frame_rate.denominator); }
    }
    if (object.contains("background") && object.at("background").is_string()) composition.background = object.at("background").get<std::string>();
    if (object.contains("environment") && object.at("environment").is_string()) composition.environment_path = object.at("environment").get<std::string>();
    if (object.contains("layers") && object.at("layers").is_array()) {
        const auto& layers = object.at("layers");
        for (std::size_t i = 0; i < layers.size(); ++i) {
            if (layers[i].is_object()) composition.layers.push_back(parse_layer(layers[i], make_path(path, "layers[" + std::to_string(i) + "]"), diagnostics));
        }
    }
    return composition;
}

AssetSpec parse_asset(const json& object, const std::string& path, DiagnosticBag& diagnostics) {
    (void)path; (void)diagnostics;
    AssetSpec asset;
    read_string(object, "id", asset.id);
    read_string(object, "type", asset.type);
    read_string(object, "source", asset.source);
    read_string(object, "path", asset.path);
    if (object.contains("alpha_mode") && object.at("alpha_mode").is_string()) asset.alpha_mode = object.at("alpha_mode").get<std::string>();
    if (asset.source.empty() && !asset.path.empty()) asset.source = asset.path;
    return asset;
}

} // namespace tachyon
