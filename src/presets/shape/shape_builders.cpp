#include "tachyon/presets/shape/shape_builders.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/shapes/shape_spec.h"
#include <string>
#include <sstream>
#include <iomanip>

namespace tachyon::presets {

namespace {

ColorSpec parse_hex_color(const std::string& hex) {
    if (hex.empty() || hex[0] != '#') return ColorSpec{255, 255, 255, 255};
    std::string h = hex.substr(1);
    if (h.size() == 6) {
        unsigned int r, g, b;
        std::sscanf(h.c_str(), "%2x%2x%2x", &r, &g, &b);
        return ColorSpec{static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g), static_cast<std::uint8_t>(b), 255};
    }
    return ColorSpec{255, 255, 255, 255};
}

LayerSpec make_shape_layer_base(const ShapeParams& p) {
    LayerSpec l;
    l.id          = "shape_layer";
    l.name        = "Shape";
    l.type_string = "shape";
    l.type        = LayerType::Shape;
    l.enabled     = true;
    l.visible     = true;
    l.start_time  = p.in_point;
    l.in_point    = p.in_point;
    l.out_point   = p.out_point;
    l.width       = static_cast<int>(p.w);
    l.height      = static_cast<int>(p.h);
    l.opacity     = static_cast<double>(p.opacity);
    l.transform.position_x = static_cast<double>(p.x);
    l.transform.position_y = static_cast<double>(p.y);
    l.shape_spec.emplace(ShapeSpec{});

    return l;
}

} // anonymous namespace

LayerSpec build_shape(const ShapeParams& p) {
    if (p.type == "circle") return build_shape_circle(p);
    if (p.type == "ellipse") return build_shape_ellipse(p);
    if (p.type == "line") return build_shape_line(p);
    return build_shape_rect(p); // default
}

LayerSpec build_shape_rect(const ShapeParams& p) {
    LayerSpec l = make_shape_layer_base(p);
    l.shape_spec->type = "rectangle";
    l.shape_spec->fill_color = parse_hex_color(p.fill_color);
    l.shape_spec->stroke_color = parse_hex_color(p.stroke_color);
    l.shape_spec->stroke_width = p.stroke_width;
    l.shape_spec->radius = p.corner_radius;
    return l;
}

LayerSpec build_shape_circle(const ShapeParams& p) {
    LayerSpec l = make_shape_layer_base(p);
    l.shape_spec->type = "circle";
    l.shape_spec->fill_color = parse_hex_color(p.fill_color);
    l.shape_spec->stroke_color = parse_hex_color(p.stroke_color);
    l.shape_spec->stroke_width = p.stroke_width;
    return l;
}

LayerSpec build_shape_ellipse(const ShapeParams& p) {
    LayerSpec l = make_shape_layer_base(p);
    l.shape_spec->type = "ellipse";
    l.shape_spec->fill_color = parse_hex_color(p.fill_color);
    l.shape_spec->stroke_color = parse_hex_color(p.stroke_color);
    l.shape_spec->stroke_width = p.stroke_width;
    return l;
}

LayerSpec build_shape_line(const ShapeParams& p) {
    LayerSpec l = make_shape_layer_base(p);
    l.shape_spec->type = "line";
    l.shape_spec->stroke_color = parse_hex_color(p.stroke_color);
    l.shape_spec->stroke_width = p.stroke_width;
    return l;
}

} // namespace tachyon::presets
