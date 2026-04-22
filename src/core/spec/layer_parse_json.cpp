#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include <algorithm>

namespace tachyon {

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

bool parse_text_highlight(const json& object, TextHighlightSpec& highlight, const std::string& path, DiagnosticBag& diagnostics) {
    if (!object.is_object()) {
        return false;
    }

    if (object.contains("start") && object.at("start").is_number_unsigned()) {
        highlight.start_glyph = object.at("start").get<std::size_t>();
    } else if (object.contains("start") && object.at("start").is_number_integer()) {
        const auto value = object.at("start").get<std::int64_t>();
        if (value < 0) {
            diagnostics.add_error("scene.layer.highlight.start_invalid", "highlight.start must be non-negative", path + ".start");
            return false;
        }
        highlight.start_glyph = static_cast<std::size_t>(value);
    }

    if (object.contains("end") && object.at("end").is_number_unsigned()) {
        highlight.end_glyph = object.at("end").get<std::size_t>();
    } else if (object.contains("end") && object.at("end").is_number_integer()) {
        const auto value = object.at("end").get<std::int64_t>();
        if (value < 0) {
            diagnostics.add_error("scene.layer.highlight.end_invalid", "highlight.end must be non-negative", path + ".end");
            return false;
        }
        highlight.end_glyph = static_cast<std::size_t>(value);
    }

    if (object.contains("color")) {
        ColorSpec color = highlight.color;
        if (parse_color_value(object.at("color"), color)) {
            highlight.color = color;
        }
    }

    if (object.contains("padding_x") && object.at("padding_x").is_number_integer()) {
        highlight.padding_x = object.at("padding_x").get<std::int32_t>();
    }
    if (object.contains("padding_y") && object.at("padding_y").is_number_integer()) {
        highlight.padding_y = object.at("padding_y").get<std::int32_t>();
    }

    return true;
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
    read_bool(object, "motion_blur", layer.motion_blur);

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
    
    // Trim Path
    parse_optional_scalar_property(object, "trim_start", layer.trim_start, path, diagnostics);
    parse_optional_scalar_property(object, "trim_end", layer.trim_end, path, diagnostics);
    parse_optional_scalar_property(object, "trim_offset", layer.trim_offset, path, diagnostics);
    
    // Repeater
    parse_optional_scalar_property(object, "repeater_count", layer.repeater_count, path, diagnostics);
    parse_optional_scalar_property(object, "repeater_offset_position_x", layer.repeater_offset_position_x, path, diagnostics);
    parse_optional_scalar_property(object, "repeater_offset_position_y", layer.repeater_offset_position_y, path, diagnostics);
    parse_optional_scalar_property(object, "repeater_offset_rotation", layer.repeater_offset_rotation, path, diagnostics);
    parse_optional_scalar_property(object, "repeater_offset_scale_x", layer.repeater_offset_scale_x, path, diagnostics);
    parse_optional_scalar_property(object, "repeater_offset_scale_y", layer.repeater_offset_scale_y, path, diagnostics);
    parse_optional_scalar_property(object, "repeater_start_opacity", layer.repeater_start_opacity, path, diagnostics);
    parse_optional_scalar_property(object, "repeater_end_opacity", layer.repeater_end_opacity, path, diagnostics);
    
    // Gradients
    if (object.contains("gradient_fill")) {
        GradientSpec grad;
        if (parse_gradient_spec(object.at("gradient_fill"), grad)) {
            layer.gradient_fill = std::move(grad);
        }
    }
    if (object.contains("gradient_stroke")) {
        GradientSpec grad;
        if (parse_gradient_spec(object.at("gradient_stroke"), grad)) {
            layer.gradient_stroke = std::move(grad);
        }
    }

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

    if (object.contains("text_highlights") && object.at("text_highlights").is_array()) {
        const auto& highlight_array = object.at("text_highlights");
        for (std::size_t hi = 0; hi < highlight_array.size(); ++hi) {
            const auto& highlight_obj = highlight_array[hi];
            if (!highlight_obj.is_object()) {
                continue;
            }

            TextHighlightSpec highlight;
            if (parse_text_highlight(highlight_obj, highlight, path + ".text_highlights[" + std::to_string(hi) + "]", diagnostics)) {
                if (highlight.end_glyph > highlight.start_glyph) {
                    layer.text_highlights.push_back(std::move(highlight));
                }
            }
        }
    }

    parse_optional_scalar_property(object, "mask_feather", layer.mask_feather, path, diagnostics);

    return layer;
}

} // namespace tachyon
