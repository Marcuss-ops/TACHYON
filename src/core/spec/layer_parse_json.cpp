#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include <algorithm>

using json = nlohmann::json;

namespace tachyon {

void parse_effects(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics);
void parse_text_animators(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics);
void parse_text_highlights(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics);
void parse_track_bindings(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics);
void parse_time_remap(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics);
void parse_transition(const json& object, LayerTransitionSpec& out, const std::string& path, DiagnosticBag& diagnostics);
void parse_shape_spec(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics);
void parse_procedural_spec(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics);

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
    
    if (out.type == "instance") {
        InstanceSpec inst;
        read_string(object, "source", inst.source);
        if (object.contains("overrides") && object.at("overrides").is_object()) {
            for (auto& [key, val] : object.at("overrides").items()) {
                inst.overrides[key] = val;
            }
        }
        out.instance = std::move(inst);
    }
    
    read_number(object, "start_time", out.start_time);
    read_number(object, "in_point", out.in_point);
    read_number(object, "out_point", out.out_point);
    read_number(object, "opacity", out.opacity);
    read_number(object, "width", out.width);
    read_number(object, "height", out.height);
    
    parse_transform(object, out, path, diagnostics);
    
    // Auto-parse scalar and color properties using X-Macros
    #define TACHYON_PROPERTY_SCALAR(json_key, cpp_field, fallback) \
        parse_optional_scalar_property(object, #json_key, out.cpp_field, path, diagnostics);
    #define TACHYON_PROPERTY_COLOR(json_key, cpp_field, fallback) \
        parse_optional_color_property(object, #json_key, out.cpp_field, path, diagnostics);
    #include "tachyon/core/spec/schema/objects/layer_properties.def"

    
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
    read_string(object, "alignment", out.alignment);
    // Variable font axes
    if (object.contains("font_axes") && object.at("font_axes").is_object()) {
        for (auto& [axis_tag, axis_value] : object.at("font_axes").items()) {
            AnimatedScalarSpec axis_spec;
            parse_optional_scalar_property(axis_value, "", axis_spec, path, diagnostics);
            out.font_axes[axis_tag] = axis_spec;
        }
    }
    read_number(object, "stroke_width", out.stroke_width);
    read_number(object, "extrusion_depth", out.extrusion_depth);

    read_number(object, "bevel_size", out.bevel_size);
    read_number(object, "hole_bevel_ratio", out.hole_bevel_ratio);
    
    // Subtitle & Word Timestamps
    read_string(object, "subtitle_path", out.subtitle_path);
    read_string(object, "word_timestamp_path", out.word_timestamp_path);
    read_number(object, "subtitle_outline_width", out.subtitle_outline_width);
    
    // Repeater and temporal properties handled by X-Macros already
    read_string(object, "repeater_type", out.repeater_type);

    
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

    // Animation transitions
    if (object.contains("transition_in")) parse_transition(object.at("transition_in"), out.transition_in, path + ".transition_in", diagnostics);
    if (object.contains("transition_out")) parse_transition(object.at("transition_out"), out.transition_out, path + ".transition_out", diagnostics);
    
    // Legacy/shorthand support
    if (object.contains("in")) {
        if (object.at("in").is_string()) out.transition_in.type = object.at("in").get<std::string>();
        else if (object.at("in").is_object()) parse_transition(object.at("in"), out.transition_in, path + ".in", diagnostics);
    }
    if (object.contains("out")) {
        if (object.at("out").is_string()) out.transition_out.type = object.at("out").get<std::string>();
        else if (object.at("out").is_object()) parse_transition(object.at("out"), out.transition_out, path + ".out", diagnostics);
    }

    read_string(object, "during", out.during_preset);
    if (object.contains("in_duration")) read_number(object, "in_duration", out.transition_in.duration);
    if (object.contains("out_duration")) read_number(object, "out_duration", out.transition_out.duration);

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
    
    parse_procedural_spec(object, out, path, diagnostics);

    out.spec_hash = compute_layer_hash(out);
}

} // namespace tachyon
