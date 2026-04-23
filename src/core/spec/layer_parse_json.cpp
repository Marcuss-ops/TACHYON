#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include <algorithm>

namespace tachyon {

void parse_transform(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    if (!object.contains("transform") || !object.at("transform").is_object()) return;
    const auto& transform = object.at("transform");
    const std::string t_path = make_path(path, "transform");
    
    parse_optional_vector_property(transform, "anchor_point", layer.transform.anchor_point, t_path, diagnostics);
    parse_optional_vector_property(transform, "position", layer.transform.position_property, t_path, diagnostics);
    parse_optional_vector_property(transform, "scale", layer.transform.scale_property, t_path, diagnostics);
    parse_optional_scalar_property(transform, "rotation", layer.transform.rotation_property, t_path, diagnostics);
    
    // Legacy support
    if (transform.contains("position_x")) read_number(transform, "position_x", layer.transform.position_x);
    if (transform.contains("position_y")) read_number(transform, "position_y", layer.transform.position_y);
    if (transform.contains("scale_x")) read_number(transform, "scale_x", layer.transform.scale_x);
    if (transform.contains("scale_y")) read_number(transform, "scale_y", layer.transform.scale_y);
    if (transform.contains("rotation")) read_number(transform, "rotation", layer.transform.rotation);
}

static bool parse_point2d(const json& arr, math::Vector2& out) {
    if (!arr.is_array() || arr.size() < 2) return false;
    out.x = arr[0].get<float>();
    out.y = arr[1].get<float>();
    return true;
}

static void parse_path_vertex(const json& obj, PathVertex& vertex) {
    if (obj.is_array() && obj.size() >= 2) {
        vertex.point.x = obj[0].get<float>();
        vertex.point.y = obj[1].get<float>();
        vertex.position = vertex.point;
        vertex.in_tangent = math::Vector2{0.0f, 0.0f};
        vertex.out_tangent = math::Vector2{0.0f, 0.0f};
        vertex.tangent_in = math::Vector2{0.0f, 0.0f};
        vertex.tangent_out = math::Vector2{0.0f, 0.0f};
    } else if (obj.is_object()) {
        if (obj.contains("position") && obj.at("position").is_array()) {
            parse_point2d(obj.at("position"), vertex.position);
            vertex.point = vertex.position;
        }
        if (obj.contains("in_tangent") && obj.at("in_tangent").is_array()) {
            parse_point2d(obj.at("in_tangent"), vertex.in_tangent);
            vertex.tangent_in = vertex.in_tangent;
        }
        if (obj.contains("out_tangent") && obj.at("out_tangent").is_array()) {
            parse_point2d(obj.at("out_tangent"), vertex.out_tangent);
            vertex.tangent_out = vertex.out_tangent;
        }
        if (obj.contains("tangent_in") && obj.at("tangent_in").is_array()) {
            parse_point2d(obj.at("tangent_in"), vertex.tangent_in);
            vertex.in_tangent = vertex.tangent_in;
        }
        if (obj.contains("tangent_out") && obj.at("tangent_out").is_array()) {
            parse_point2d(obj.at("tangent_out"), vertex.tangent_out);
            vertex.out_tangent = vertex.tangent_out;
        }
    }
}

void parse_shape_path(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    (void)path;
    if (!object.contains("shape_path")) return;
    const auto& sp = object.at("shape_path");

    // Legacy string format: warn and ignore
    if (sp.is_string()) {
        diagnostics.add_warning("layer.shape_path.legacy_string", "shape_path as string is deprecated; use object format");
        return;
    }

    if (!sp.is_object()) {
        diagnostics.add_error("layer.shape_path.invalid", "shape_path must be an object");
        return;
    }

    ShapePathSpec spec;
    read_bool(sp, "closed", spec.closed);

    if (sp.contains("points") && sp.at("points").is_array()) {
        for (const auto& p : sp.at("points")) {
            PathVertex vertex;
            parse_path_vertex(p, vertex);
            spec.points.push_back(std::move(vertex));
        }
    }

    if (sp.contains("subpaths") && sp.at("subpaths").is_array()) {
        for (const auto& s : sp.at("subpaths")) {
            if (!s.is_object()) continue;
            ShapeSubpath sub;
            read_bool(s, "closed", sub.closed);
            if (s.contains("vertices") && s.at("vertices").is_array()) {
                for (const auto& v : s.at("vertices")) {
                    PathVertex vertex;
                    parse_path_vertex(v, vertex);
                    sub.vertices.push_back(std::move(vertex));
                }
            }
            spec.subpaths.push_back(std::move(sub));
        }
    }

    layer.shape_path = std::move(spec);
}

void parse_effects(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    (void)path; (void)diagnostics;
    if (object.contains("effects") && object.at("effects").is_array()) {
        for (const auto& effect : object.at("effects")) {
            if (effect.is_string()) layer.effects.push_back(effect.get<std::string>());
        }
    }
}

void parse_text_animators(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    (void)path; (void)diagnostics;
    if (object.contains("text_animators") && object.at("text_animators").is_array()) {
        for (const auto& anim : object.at("text_animators")) {
            if (anim.is_string()) layer.text_animators.push_back(anim.get<std::string>());
        }
    }
}

void parse_text_highlights(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    (void)path; (void)diagnostics;
    if (object.contains("text_highlights") && object.at("text_highlights").is_array()) {
        for (const auto& high : object.at("text_highlights")) {
            if (high.is_string()) layer.text_highlights.push_back(high.get<std::string>());
        }
    }
}

void parse_track_bindings(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    (void)path; (void)diagnostics;
    if (!object.contains("track_bindings") || !object.at("track_bindings").is_array()) return;
    const auto& bindings = object.at("track_bindings");
    for (std::size_t i = 0; i < bindings.size(); ++i) {
        if (!bindings[i].is_object()) continue;
        const auto& b = bindings[i];
        spec::TrackBinding binding;
        read_string(b, "property_path", binding.property_path);
        read_string(b, "source_id", binding.source_id);
        read_string(b, "source_track_name", binding.source_track_name);
        read_number(b, "influence", binding.influence);
        read_bool(b, "enabled", binding.enabled);
        layer.track_bindings.push_back(std::move(binding));
    }
}

void parse_time_remap(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    (void)path; (void)diagnostics;
    if (!object.contains("time_remap") || !object.at("time_remap").is_object()) return;
    const auto& tr = object.at("time_remap");
    read_bool(tr, "enabled", layer.time_remap.enabled);
    
    if (tr.contains("mode") && tr.at("mode").is_string()) {
        const std::string mode = tr.at("mode").get<std::string>();
        if (mode == "hold") layer.time_remap.mode = spec::TimeRemapMode::Hold;
        else if (mode == "blend") layer.time_remap.mode = spec::TimeRemapMode::Blend;
        else if (mode == "optical_flow") layer.time_remap.mode = spec::TimeRemapMode::OpticalFlow;
    }
    
    if (tr.contains("keyframes") && tr.at("keyframes").is_array()) {
        const auto& kfs = tr.at("keyframes");
        for (const auto& kf : kfs) {
            if (kf.is_array() && kf.size() >= 2) {
                layer.time_remap.keyframes.push_back({kf[0].get<float>(), kf[1].get<float>()});
            } else if (kf.is_object()) {
                float src = 0.0f, dest = 0.0f;
                read_number(kf, "source_time", src);
                read_number(kf, "dest_time", dest);
                layer.time_remap.keyframes.push_back({src, dest});
            }
        }
    }
}

void parse_layer(const json& object, LayerSpec& out, const std::string& path, DiagnosticBag& diagnostics) {
    read_string(object, "id", out.id);
    read_string(object, "name", out.name);
    read_string(object, "type", out.type);
    read_string(object, "blend_mode", out.blend_mode);
    read_bool(object, "enabled", out.enabled);
    read_bool(object, "visible", out.visible);
    read_bool(object, "is_3d", out.is_3d);
    read_bool(object, "is_adjustment_layer", out.is_adjustment_layer);
    read_bool(object, "motion_blur", out.motion_blur);
    
    read_number(object, "start_time", out.start_time);
    read_number(object, "in_point", out.in_point);
    read_number(object, "out_point", out.out_point);
    read_number(object, "opacity", out.opacity);
    read_number(object, "width", out.width);
    read_number(object, "height", out.height);
    
    parse_transform(object, out, path, diagnostics);
    parse_optional_scalar_property(object, "opacity", out.opacity_property, path, diagnostics);
    parse_optional_scalar_property(object, "mask_feather", out.mask_feather, path, diagnostics);
    
    parse_shape_path(object, out, path, diagnostics);
    parse_effects(object, out, path, diagnostics);
    parse_text_animators(object, out, path, diagnostics);
    parse_text_highlights(object, out, path, diagnostics);

    if (object.contains("time_remap") && object.at("time_remap").is_number()) {
        out.time_remap_property.value = object.at("time_remap").get<double>();
    }
    
    if (object.contains("parent") && object.at("parent").is_string()) out.parent = object.at("parent").get<std::string>();
    if (object.contains("track_matte_layer_id") && object.at("track_matte_layer_id").is_string()) out.track_matte_layer_id = object.at("track_matte_layer_id").get<std::string>();
    if (object.contains("track_matte_type") && object.at("track_matte_type").is_string()) {
        const std::string mt = object.at("track_matte_type").get<std::string>();
        if (mt == "alpha") out.track_matte_type = TrackMatteType::Alpha;
        else if (mt == "alpha_inverted") out.track_matte_type = TrackMatteType::AlphaInverted;
        else if (mt == "luma") out.track_matte_type = TrackMatteType::Luma;
        else if (mt == "luma_inverted") out.track_matte_type = TrackMatteType::LumaInverted;
    }
    if (object.contains("precomp_id") && object.at("precomp_id").is_string()) out.precomp_id = object.at("precomp_id").get<std::string>();
    
    // Text
    read_string(object, "text_content", out.text_content);
    read_string(object, "font_id", out.font_id);
    parse_optional_scalar_property(object, "font_size", out.font_size, path, diagnostics);
    read_string(object, "alignment", out.alignment);
    parse_optional_color_property(object, "fill_color", out.fill_color, path, diagnostics);
    parse_optional_color_property(object, "stroke_color", out.stroke_color, path, diagnostics);
    read_number(object, "stroke_width", out.stroke_width);
    parse_optional_scalar_property(object, "stroke_width", out.stroke_width_property, path, diagnostics);
    
    // Repeater
    parse_optional_scalar_property(object, "repeater_count", out.repeater_count, path, diagnostics);
    parse_optional_scalar_property(object, "repeater_stagger_delay", out.repeater_stagger_delay, path, diagnostics);
    parse_optional_scalar_property(object, "repeater_offset_position_x", out.repeater_offset_position_x, path, diagnostics);
    parse_optional_scalar_property(object, "repeater_offset_position_y", out.repeater_offset_position_y, path, diagnostics);
    parse_optional_scalar_property(object, "repeater_offset_rotation", out.repeater_offset_rotation, path, diagnostics);
    parse_optional_scalar_property(object, "repeater_offset_scale_x", out.repeater_offset_scale_x, path, diagnostics);
    parse_optional_scalar_property(object, "repeater_offset_scale_y", out.repeater_offset_scale_y, path, diagnostics);
    parse_optional_scalar_property(object, "repeater_start_opacity", out.repeater_start_opacity, path, diagnostics);
    parse_optional_scalar_property(object, "repeater_end_opacity", out.repeater_end_opacity, path, diagnostics);

    // Temporal & Tracking
    parse_track_bindings(object, out, path, diagnostics);
    parse_time_remap(object, out, path, diagnostics);
    
    if (object.contains("frame_blend") && object.at("frame_blend").is_string()) {
        const std::string fb = object.at("frame_blend").get<std::string>();
        if (fb == "none") out.frame_blend = spec::FrameBlendMode::None;
        else if (fb == "linear") out.frame_blend = spec::FrameBlendMode::Linear;
        else if (fb == "pixel_motion") out.frame_blend = spec::FrameBlendMode::PixelMotion;
        else if (fb == "optical_flow") out.frame_blend = spec::FrameBlendMode::OpticalFlow;
    }
    
    // Camera
    read_string(object, "camera_type", out.camera_type);
    parse_optional_scalar_property(object, "camera_zoom", out.camera_zoom, path, diagnostics);
    parse_optional_vector3_property(object, "camera_poi", out.camera_poi, path, diagnostics);
    
    // Camera Shake
    if (object.contains("camera_shake")) {
        const auto& cs = object.at("camera_shake");
        read_number(cs, "seed", out.camera_shake_seed);
        parse_optional_scalar_property(cs, "amplitude_pos", out.camera_shake_amplitude_pos, path + ".camera_shake", diagnostics);
        parse_optional_scalar_property(cs, "amplitude_rot", out.camera_shake_amplitude_rot, path + ".camera_shake", diagnostics);
        parse_optional_scalar_property(cs, "frequency", out.camera_shake_frequency, path + ".camera_shake", diagnostics);
        parse_optional_scalar_property(cs, "roughness", out.camera_shake_roughness, path + ".camera_shake", diagnostics);
    }
}

} // namespace tachyon
