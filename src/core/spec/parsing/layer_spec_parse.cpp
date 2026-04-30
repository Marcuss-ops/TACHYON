#include "tachyon/core/spec/schema/objects/scene_spec_core.h"

using json = nlohmann::json;

namespace tachyon {

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

void parse_procedural_spec(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    if (!object.contains("procedural") || !object.at("procedural").is_object()) return;
    const auto& p = object.at("procedural");
    const std::string p_path = path + ".procedural";
    
    ProceduralSpec spec;
    read_string(p, "kind", spec.kind);
    read_number(p, "seed", spec.seed);
    
    // Core
    parse_optional_scalar_property(p, "frequency", spec.frequency, p_path, diagnostics);
    parse_optional_scalar_property(p, "speed", spec.speed, p_path, diagnostics);
    parse_optional_scalar_property(p, "amplitude", spec.amplitude, p_path, diagnostics);
    parse_optional_scalar_property(p, "scale", spec.scale, p_path, diagnostics);
    parse_optional_scalar_property(p, "angle", spec.angle, p_path, diagnostics);
    
    // Colors
    parse_optional_color_property(p, "color_a", spec.color_a, p_path, diagnostics);
    parse_optional_color_property(p, "color_b", spec.color_b, p_path, diagnostics);
    parse_optional_color_property(p, "color_c", spec.color_c, p_path, diagnostics);
    
    // Grid & Geometry
    read_string(p, "shape", spec.shape);
    parse_optional_scalar_property(p, "spacing", spec.spacing, p_path, diagnostics);
    parse_optional_scalar_property(p, "border_width", spec.border_width, p_path, diagnostics);
    parse_optional_color_property(p, "border_color", spec.border_color, p_path, diagnostics);
    
    // Warp
    parse_optional_scalar_property(p, "warp_strength", spec.warp_strength, p_path, diagnostics);
    parse_optional_scalar_property(p, "warp_frequency", spec.warp_frequency, p_path, diagnostics);
    parse_optional_scalar_property(p, "warp_speed", spec.warp_speed, p_path, diagnostics);
    
    // Post-Process
    parse_optional_scalar_property(p, "grain_amount", spec.grain_amount, p_path, diagnostics);
    parse_optional_scalar_property(p, "grain_scale", spec.grain_scale, p_path, diagnostics);
    parse_optional_scalar_property(p, "scanline_intensity", spec.scanline_intensity, p_path, diagnostics);
    parse_optional_scalar_property(p, "scanline_frequency", spec.scanline_frequency, p_path, diagnostics);
    parse_optional_scalar_property(p, "contrast", spec.contrast, p_path, diagnostics);
    parse_optional_scalar_property(p, "gamma", spec.gamma, p_path, diagnostics);
    parse_optional_scalar_property(p, "saturation", spec.saturation, p_path, diagnostics);
    parse_optional_scalar_property(p, "softness", spec.softness, p_path, diagnostics);

    // Advanced
    parse_optional_scalar_property(p, "octave_decay", spec.octave_decay, p_path, diagnostics);
    parse_optional_scalar_property(p, "band_height", spec.band_height, p_path, diagnostics);
    parse_optional_scalar_property(p, "band_spread", spec.band_spread, p_path, diagnostics);
    
    layer.procedural = std::move(spec);
}

} // namespace tachyon
