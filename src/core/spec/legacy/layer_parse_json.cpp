#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/core/scene/evaluator/layer_utils.h"
#include <algorithm>

namespace tachyon {

namespace {

void apply_transform_modern(const json& transform, LayerSpec& layer,
    const std::string& t_path, DiagnosticBag& diagnostics) {
    parse_optional_vector_property(transform, "anchor_point", layer.transform.anchor_point, t_path, diagnostics);
    parse_optional_vector_property(transform, "position", layer.transform.position_property, t_path, diagnostics);
    parse_optional_vector_property(transform, "scale", layer.transform.scale_property, t_path, diagnostics);
    parse_optional_scalar_property(transform, "rotation", layer.transform.rotation_property, t_path, diagnostics);
}

void apply_transform_legacy(const json& transform, LayerSpec& layer) {
    if (transform.contains("position_x")) read_number(transform, "position_x", layer.transform.position_x);
    if (transform.contains("position_y")) read_number(transform, "position_y", layer.transform.position_y);
    if (transform.contains("scale_x")) read_number(transform, "scale_x", layer.transform.scale_x);
    if (transform.contains("scale_y")) read_number(transform, "scale_y", layer.transform.scale_y);
    if (transform.contains("rotation")) read_number(transform, "rotation", layer.transform.rotation);
}

} // namespace

void parse_transform(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    if (!object.contains("transform") || !object.at("transform").is_object()) return;
    const auto& transform = object.at("transform");
    const std::string t_path = make_path(path, "transform");
    
    apply_transform_modern(transform, layer, t_path, diagnostics);
    apply_transform_legacy(transform, layer);
}

void parse_transform3d(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    if (!object.contains("transform3d") || !object.at("transform3d").is_object()) return;
    const auto& transform = object.at("transform3d");
    const std::string t_path = make_path(path, "transform3d");
    
    parse_optional_vector3_property(transform, "anchor_point", layer.transform3d.anchor_point_property, t_path, diagnostics);
    parse_optional_vector3_property(transform, "position", layer.transform3d.position_property, t_path, diagnostics);
    parse_optional_vector3_property(transform, "orientation", layer.transform3d.orientation_property, t_path, diagnostics);
    parse_optional_vector3_property(transform, "rotation", layer.transform3d.rotation_property, t_path, diagnostics);
    parse_optional_vector3_property(transform, "scale", layer.transform3d.scale_property, t_path, diagnostics);
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
            if (!effect.is_object()) continue;
            EffectSpec spec;
            read_string(effect, "type", spec.type);
            // Set kind from type string
            if (!spec.type.empty()) {
                // spec.kind = effect_kind_from_string(spec.type); // obsolete
            }
            read_bool(effect, "enabled", spec.enabled);
            if (effect.contains("scalars") && effect.at("scalars").is_object()) {
                for (auto& [key, val] : effect.at("scalars").items()) {
                    if (val.is_number()) spec.scalars[key] = val.get<double>();
                }
            }
            if (effect.contains("colors") && effect.at("colors").is_object()) {
                for (auto& [key, val] : effect.at("colors").items()) {
                    if (val.is_object()) {
                        ColorSpec color;
                        read_number(val, "r", color.r);
                        read_number(val, "g", color.g);
                        read_number(val, "b", color.b);
                        read_number(val, "a", color.a);
                        spec.colors[key] = color;
                    }
                }
            }
            if (effect.contains("strings") && effect.at("strings").is_object()) {
                for (auto& [key, val] : effect.at("strings").items()) {
                    if (val.is_string()) spec.strings[key] = val.get<std::string>();
                }
            }
            layer.effects.push_back(std::move(spec));
        }
    }
}

void parse_text_animators(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    (void)path; (void)diagnostics;
    if (object.contains("text_animators") && object.at("text_animators").is_array()) {
        for (const auto& anim : object.at("text_animators")) {
            if (anim.is_object()) {
                TextAnimatorSpec spec;
                from_json(anim, spec);
                layer.text_animators.push_back(std::move(spec));
            }
        }
    }
}

void parse_text_highlights(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    (void)path; (void)diagnostics;
    if (object.contains("text_highlights") && object.at("text_highlights").is_array()) {
        for (const auto& high : object.at("text_highlights")) {
            if (high.is_object()) {
                TextHighlightSpec spec;
                from_json(high, spec);
                layer.text_highlights.push_back(std::move(spec));
            }
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

void parse_shape_spec(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    (void)path; (void)diagnostics;
    if (object.contains("shape_type")) {
        ShapeSpec ss;
        read_string(object, "shape_type", ss.type);
        read_number(object, "x", ss.x);
        read_number(object, "y", ss.y);
        read_number(object, "width", ss.width);
        read_number(object, "height", ss.height);
        read_number(object, "radius", ss.radius);
        read_number(object, "sides", ss.sides);
        read_number(object, "inner_radius", ss.inner_radius);
        read_number(object, "outer_radius", ss.outer_radius);
        read_number(object, "x1", ss.x1);
        read_number(object, "y1", ss.y1);
        read_number(object, "head_size", ss.head_size);
        read_number(object, "tail_x", ss.tail_x);
        read_number(object, "tail_y", ss.tail_y);
        read_number(object, "stroke_width", ss.stroke_width);
        read_string(object, "line_cap", ss.line_cap);
        read_string(object, "line_join", ss.line_join);
        read_number(object, "dash_offset", ss.dash_offset);
        if (object.contains("dash_array") && object.at("dash_array").is_array()) {
            for (const auto& dash : object.at("dash_array")) {
                if (dash.is_number()) ss.dash_array.push_back(dash.get<float>());
            }
        }
        
        ColorSpec c_fill, c_stroke;
        if (parse_color_value(object.value("fill_color", json::array()), c_fill)) ss.fill_color = c_fill;
        if (parse_color_value(object.value("stroke_color", json::array()), c_stroke)) ss.stroke_color = c_stroke;

        if (object.contains("gradient_fill")) {
            GradientSpec grad;
            if (parse_gradient_spec(object.at("gradient_fill"), grad)) {
                ss.gradient_fill = grad;
            }
        }

        if (object.contains("gradient_stroke")) {
            GradientSpec grad;
            if (parse_gradient_spec(object.at("gradient_stroke"), grad)) {
                ss.gradient_stroke = grad;
            }
        }

        layer.shape_spec = ss;
    }
}

void parse_layer(const json& object, LayerSpec& out, const std::string& path, DiagnosticBag& diagnostics) {
    read_string(object, "id", out.id);
    read_string(object, "name", out.name);
    read_string(object, "type", out.type);
    read_string(object, "asset_id", out.asset_id);
    // Set kind from type string
    if (!out.type.empty()) {
        out.kind = scene::map_layer_type(out.type);
    }
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
    parse_transform3d(object, out, path, diagnostics);
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
    
    // Subtitle & Word Timestamps
    read_string(object, "subtitle_path", out.subtitle_path);
    read_string(object, "word_timestamp_path", out.word_timestamp_path);
    
    read_number(object, "subtitle_outline_width", out.subtitle_outline_width);
    parse_optional_color_property(object, "subtitle_outline_color", out.subtitle_outline_color, path, diagnostics);
    
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

    // Timing shorthand
    read_number(object, "duration", out.duration);

    // Animation presets
    read_string(object, "in", out.in_preset);
    read_string(object, "during", out.during_preset);
    read_string(object, "out", out.out_preset);
    read_number(object, "in_duration", out.in_duration);
    read_number(object, "out_duration", out.out_duration);

    // Playback behavior
    read_bool(object, "loop", out.loop);
    read_bool(object, "hold_last_frame", out.hold_last_frame);

    // Markers
    if (object.contains("markers") && object.at("markers").is_array()) {
        const auto& markers = object.at("markers");
        for (const auto& m : markers) {
            if (!m.is_object()) continue;
            LayerSpec::MarkerSpec marker;
            read_number(m, "time", marker.time);
            read_string(m, "label", marker.label);
            read_string(m, "color", marker.color);
            out.markers.push_back(std::move(marker));
        }
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

void parse_mask_paths(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    (void)path; (void)diagnostics;
    if (!object.contains("mask_paths") || !object.at("mask_paths").is_array()) return;
    const auto& mask_paths = object.at("mask_paths");
    for (std::size_t i = 0; i < mask_paths.size(); ++i) {
        if (!mask_paths[i].is_object()) continue;
        const auto& mp = mask_paths[i];
        renderer2d::MaskPath mask_path;
        if (mp.contains("vertices") && mp.at("vertices").is_array()) {
            const auto& vertices = mp.at("vertices");
            for (std::size_t v = 0; v < vertices.size(); ++v) {
                if (!vertices[v].is_object()) continue;
                const auto& vert = vertices[v];
                renderer2d::MaskVertex vertex;
                if (vert.contains("position") && vert.at("position").is_array() && vert.at("position").size() >= 2) {
                    vertex.position.x = vert.at("position")[0].get<float>();
                    vertex.position.y = vert.at("position")[1].get<float>();
                }
                if (vert.contains("in_tangent") && vert.at("in_tangent").is_array() && vert.at("in_tangent").size() >= 2) {
                    vertex.in_tangent.x = vert.at("in_tangent")[0].get<float>();
                    vertex.in_tangent.y = vert.at("in_tangent")[1].get<float>();
                }
                if (vert.contains("out_tangent") && vert.at("out_tangent").is_array() && vert.at("out_tangent").size() >= 2) {
                    vertex.out_tangent.x = vert.at("out_tangent")[0].get<float>();
                    vertex.out_tangent.y = vert.at("out_tangent")[1].get<float>();
                }
                read_number(vert, "feather_inner", vertex.feather_inner);
                read_number(vert, "feather_outer", vertex.feather_outer);
                mask_path.vertices.push_back(std::move(vertex));
            }
        }
        if (mp.contains("is_closed") && mp.at("is_closed").is_boolean()) mask_path.is_closed = mp.at("is_closed").get<bool>();
        if (mp.contains("is_inverted") && mp.at("is_inverted").is_boolean()) mask_path.is_inverted = mp.at("is_inverted").get<bool>();
        layer.mask_paths.push_back(std::move(mask_path));
    }
}

} // namespace tachyon
