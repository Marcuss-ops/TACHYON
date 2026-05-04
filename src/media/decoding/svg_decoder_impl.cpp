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
                // TODO: parse gradient stops and coordinates
                out_result.gradients.push_back(gs);
            } else if (strcmp(grad.name(), "radialGradient") == 0) {
                GradientSpec gs;
                gs.type = GradientType::Radial;
                // TODO: parse gradient stops and radius
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
                // TODO: parse color string to Color
            }
            out_result.fill_styles.push_back(fill);

            // Parse stroke
            renderer2d::StrokePathStyle stroke;
            const char* stroke_attr = child.attribute("stroke").value();
            if (stroke_attr && strcmp(stroke_attr, "none") != 0) {
                // TODO: parse stroke properties
            }
            const char* stroke_width = child.attribute("stroke-width").value();
            if (stroke_width) {
                stroke.stroke_width = std::stof(stroke_width);
            }
            out_result.stroke_styles.push_back(stroke);
        }
    }

    return true;
}

} // namespace media
} // namespace tachyon
