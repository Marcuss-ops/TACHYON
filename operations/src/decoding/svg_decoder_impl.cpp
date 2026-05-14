#include "tachyon/media/decoding/svg_decoder.h"
#include "pugixml.hpp"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include <optional>
#include <sstream>
#include <vector>
#include <cmath>
#include <cstring>

namespace tachyon {
namespace media {

namespace {

// Helper to parse SVG path d attribute to PathGeometry
bool parse_path_d(const std::string& d, renderer2d::PathGeometry& out_geometry, DiagnosticBag& diag) {
    // Minimal SVG path parser: handles M, L, C, Z (absolute only initially)
    std::istringstream iss(d);
    char cmd = '\0';
    bool has_cmd = false;
    float x = 0, y = 0, x1 = 0, y1 = 0, x2 = 0, y2 = 0;

    while (iss >> cmd || has_cmd) {
        if (!has_cmd) {
            has_cmd = true;
        }
        switch (cmd) {
            case 'M': { // MoveTo
                if (!(iss >> x >> y)) {
                    diag.diagnostics.push_back({DiagnosticSeverity::Error, "svg_parse", "", "Invalid M command arguments", ""});
                    return false;
                }
                out_geometry.commands.push_back({renderer2d::PathVerb::MoveTo, {x, y}, {}, {}});
                break;
            }
            case 'L': { // LineTo
                if (!(iss >> x >> y)) {
                    diag.diagnostics.push_back({DiagnosticSeverity::Error, "svg_parse", "", "Invalid L command arguments", ""});
                    return false;
                }
                out_geometry.commands.push_back({renderer2d::PathVerb::LineTo, {x, y}, {}, {}});
                break;
            }
            case 'C': { // CubicTo
                if (!(iss >> x1 >> y1 >> x2 >> y2 >> x >> y)) {
                    diag.diagnostics.push_back({DiagnosticSeverity::Error, "svg_parse", "", "Invalid C command arguments", ""});
                    return false;
                }
                renderer2d::PathCommand pc;
                pc.verb = renderer2d::PathVerb::CubicTo;
                pc.p0 = {x, y};
                pc.p1 = {x1, y1};
                pc.p2 = {x2, y2};
                out_geometry.commands.push_back(pc);
                break;
            }
            case 'Z': { // Close
                out_geometry.commands.push_back({renderer2d::PathVerb::Close, {}, {}, {}});
                break;
            }
            default: {
                diag.diagnostics.push_back({DiagnosticSeverity::Error, "svg_parse", "", "Unsupported path command: " + std::string(1, cmd), ""});
                return false;
            }
        }
        // Check next char if it's a command
        char next_cmd;
        if (iss >> next_cmd && isalpha(next_cmd)) {
            cmd = next_cmd;
            has_cmd = false;
        } else {
            iss.unget();
        }
    }
    return true;
}

// Convert PathGeometry to ShapePathSpec
shapes::ShapePathSpec path_geometry_to_shape(const renderer2d::PathGeometry& geom) {
    shapes::ShapePathSpec spec;
    for (const auto& cmd : geom.commands) {
        shapes::PathVertex v;
        v.point = cmd.p0;
        v.in_tangent = cmd.p1;
        v.out_tangent = cmd.p2;
        spec.points.push_back(v);
        if (cmd.verb == renderer2d::PathVerb::Close) {
            spec.closed = true;
        }
    }
    return spec;
}

// Helper to parse hex or rgb color strings
renderer2d::Color parse_color_string(const std::string& str) {
    if (str.empty() || str == "none") return renderer2d::Color::transparent();
    if (str[0] == '#') {
        std::string hex = str.substr(1);
        if (hex.length() == 3) {
            float r = std::stoi(hex.substr(0, 1), nullptr, 16) / 15.0f;
            float g = std::stoi(hex.substr(1, 1), nullptr, 16) / 15.0f;
            float b = std::stoi(hex.substr(2, 1), nullptr, 16) / 15.0f;
            return {r, g, b, 1.0f};
        } else if (hex.length() == 6) {
            float r = std::stoi(hex.substr(0, 2), nullptr, 16) / 255.0f;
            float g = std::stoi(hex.substr(2, 2), nullptr, 16) / 255.0f;
            float b = std::stoi(hex.substr(4, 2), nullptr, 16) / 255.0f;
            return {r, g, b, 1.0f};
        }
    } else if (str.find("rgb") == 0) {
        // Simple rgb(r,g,b) parser
        size_t start = str.find('(') + 1;
        size_t end = str.find(')');
        std::string vals = str.substr(start, end - start);
        std::stringstream ss(vals);
        float r, g, b;
        char comma;
        if (ss >> r >> comma >> g >> comma >> b) {
            return {r / 255.0f, g / 255.0f, b / 255.0f, 1.0f};
        }
    }
    return renderer2d::Color::black();
}

void parse_gradient_stops(pugi::xml_node node, GradientSpec& gs) {
    auto to_color_spec = [](const renderer2d::Color& c) -> ColorSpec {
        return {
            static_cast<uint8_t>(std::clamp(c.r * 255.0f, 0.0f, 255.0f)),
            static_cast<uint8_t>(std::clamp(c.g * 255.0f, 0.0f, 255.0f)),
            static_cast<uint8_t>(std::clamp(c.b * 255.0f, 0.0f, 255.0f)),
            static_cast<uint8_t>(std::clamp(c.a * 255.0f, 0.0f, 255.0f))
        };
    };

    for (pugi::xml_node stop : node.children("stop")) {
        GradientStop spec_stop;
        const char* offset = stop.attribute("offset").value();
        if (offset) {
            std::string s(offset);
            if (!s.empty() && s.back() == '%') {
                spec_stop.offset = std::stof(s.substr(0, s.size() - 1)) / 100.0f;
            } else {
                spec_stop.offset = std::stof(s);
            }
        }
        const char* stop_color = stop.attribute("stop-color").value();
        if (stop_color) {
            spec_stop.color = to_color_spec(parse_color_string(stop_color));
        }
        const char* stop_opacity = stop.attribute("stop-opacity").value();
        if (stop_opacity) {
            spec_stop.color.a = static_cast<uint8_t>(std::clamp(std::stof(stop_opacity) * 255.0f, 0.0f, 255.0f));
        }
        gs.stops.push_back(spec_stop);
    }
}

} // anonymous namespace

bool parse_svg_string(const std::string& svg_content, ParsedSvg& out_result, DiagnosticBag& diagnostics) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(svg_content.c_str());
    if (!result) {
        diagnostics.diagnostics.push_back({DiagnosticSeverity::Error, "svg_parse", "", "SVG parse failed: " + std::string(result.description()), ""});
        return false;
    }

    pugi::xml_node svg = doc.child("svg");
    if (!svg) {
        diagnostics.diagnostics.push_back({DiagnosticSeverity::Error, "svg_parse", "", "No root <svg> element found", ""});
        return false;
    }

    // Parse gradient definitions first
    pugi::xml_node defs = svg.child("defs");
    if (defs) {
        for (pugi::xml_node grad : defs.children()) {
            if (strcmp(grad.name(), "linearGradient") == 0) {
                GradientSpec gs;
                gs.type = GradientType::Linear;
                gs.start.x = grad.attribute("x1").as_float(0.0f);
                gs.start.y = grad.attribute("y1").as_float(0.0f);
                gs.end.x = grad.attribute("x2").as_float(100.0f);
                gs.end.y = grad.attribute("y2").as_float(0.0f);
                parse_gradient_stops(grad, gs);
                out_result.gradients.push_back(gs);
            } else if (strcmp(grad.name(), "radialGradient") == 0) {
                GradientSpec gs;
                gs.type = GradientType::Radial;
                gs.start.x = grad.attribute("cx").as_float(50.0f);
                gs.start.y = grad.attribute("cy").as_float(50.0f);
                gs.radial_radius = grad.attribute("r").as_float(50.0f);
                parse_gradient_stops(grad, gs);
                out_result.gradients.push_back(gs);
            }
        }
    }

    // Parse top-level elements
    for (pugi::xml_node child : svg.children()) {
        if (strcmp(child.name(), "path") == 0) {
            const char* d = child.attribute("d").value();
            if (!d || strlen(d) == 0) continue;

            renderer2d::PathGeometry geom;
            if (!parse_path_d(d, geom, diagnostics)) {
                return false;
            }

            shapes::ShapePathSpec shape = path_geometry_to_shape(geom);
            out_result.shapes.push_back(shape);

            // Parse fill
            renderer2d::FillPathStyle fill;
            const char* fill_attr = child.attribute("fill").value();
            if (fill_attr && strcmp(fill_attr, "none") != 0) {
                if (fill_attr[0] == 'u' && strstr(fill_attr, "url(#")) {
                    // TODO: handle gradient URLs
                } else {
                    fill.fill_color = parse_color_string(fill_attr);
                }
            }
            out_result.fill_styles.push_back(fill);

            // Parse stroke
            renderer2d::StrokePathStyle stroke;
            const char* stroke_attr = child.attribute("stroke").value();
            if (stroke_attr && strcmp(stroke_attr, "none") != 0) {
                stroke.stroke_color = parse_color_string(stroke_attr);
            }
            const char* stroke_width = child.attribute("stroke-width").value();
            if (stroke_width) {
                stroke.stroke_width = std::stof(stroke_width);
            }
            const char* linecap = child.attribute("stroke-linecap").value();
            if (linecap) {
                if (strcmp(linecap, "round") == 0) stroke.cap = renderer2d::LineCap::Round;
                else if (strcmp(linecap, "square") == 0) stroke.cap = renderer2d::LineCap::Square;
                else stroke.cap = renderer2d::LineCap::Butt;
            }
            const char* linejoin = child.attribute("stroke-linejoin").value();
            if (linejoin) {
                if (strcmp(linejoin, "round") == 0) stroke.join = renderer2d::LineJoin::Round;
                else if (strcmp(linejoin, "bevel") == 0) stroke.join = renderer2d::LineJoin::Bevel;
                else stroke.join = renderer2d::LineJoin::Miter;
            }
            const char* miterlimit = child.attribute("stroke-miterlimit").value();
            if (miterlimit) {
                stroke.miter_limit = std::stof(miterlimit);
            }
            out_result.stroke_styles.push_back(stroke);
        }
    }

    return true;
}

} // namespace media
} // namespace tachyon
