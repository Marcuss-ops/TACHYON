#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include <algorithm>

using json = nlohmann::json;

namespace tachyon {

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
