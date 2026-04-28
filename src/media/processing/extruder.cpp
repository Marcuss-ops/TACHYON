#include "tachyon/media/processing/extruder.h"
#include <mapbox/earcut.hpp>
#include <array>
#include <algorithm>
#include <cmath>

namespace tachyon::media {

using Point2D = std::array<float, 2>;
using PolygonType = std::vector<std::vector<Point2D>>;

namespace {
    math::Vector2 eval_bezier(const math::Vector2& p0, const math::Vector2& p1, const math::Vector2& p2, const math::Vector2& p3, float t) {
        float u = 1.0f - t;
        float tt = t * t;
        float uu = u * u;
        float uuu = uu * u;
        float ttt = tt * t;
        math::Vector2 p = p0 * uuu;
        p += p1 * 3.0f * uu * t;
        p += p2 * 3.0f * u * tt;
        p += p3 * ttt;
        return p;
    }

    void flatten_bezier(
        const math::Vector2& p0, const math::Vector2& p1, const math::Vector2& p2, const math::Vector2& p3,
        std::vector<math::Vector2>& out_points) 
    {
        // Smooth subdivision threshold
        const int steps = 16;
        for (int i = 1; i <= steps; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(steps);
            out_points.push_back(eval_bezier(p0, p1, p2, p3, t));
        }
    }

    float signed_area(const std::vector<math::Vector2>& contour) {
        if (contour.size() < 3U) {
            return 0.0f;
        }
        float area = 0.0f;
        for (std::size_t i = 0, j = contour.size() - 1; i < contour.size(); j = i++) {
            area += contour[j].x * contour[i].y - contour[i].x * contour[j].y;
        }
        return area * 0.5f;
    }

    math::Vector2 contour_centroid(const std::vector<math::Vector2>& contour) {
        math::Vector2 sum{0.0f, 0.0f};
        if (contour.empty()) {
            return sum;
        }
        for (const auto& p : contour) {
            sum += p;
        }
        return sum / static_cast<float>(contour.size());
    }

    bool point_in_polygon(const math::Vector2& p, const std::vector<math::Vector2>& contour) {
        if (contour.size() < 3U) {
            return false;
        }
        bool inside = false;
        for (std::size_t i = 0, j = contour.size() - 1; i < contour.size(); j = i++) {
            const auto& a = contour[i];
            const auto& b = contour[j];
            const bool intersect = ((a.y > p.y) != (b.y > p.y)) &&
                (p.x < (b.x - a.x) * (p.y - a.y) / ((b.y - a.y) == 0.0f ? 1.0e-8f : (b.y - a.y)) + a.x);
            if (intersect) {
                inside = !inside;
            }
        }
        return inside;
    }

    std::vector<math::Vector2> flatten_shape_path(const scene::EvaluatedShapePath& path) {
        std::vector<math::Vector2> contour;
        if (path.points.empty()) {
            return contour;
        }

        contour.push_back(path.points[0].position);

        for (std::size_t i = 1; i < path.points.size(); ++i) {
            const auto& pt_a = path.points[i - 1];
            const auto& pt_b = path.points[i];

            math::Vector2 p0 = pt_a.position;
            math::Vector2 p1 = p0 + pt_a.tangent_out;
            math::Vector2 p3 = pt_b.position;
            math::Vector2 p2 = p3 + pt_b.tangent_in;

            if (pt_a.tangent_out.length_squared() < 0.001f && pt_b.tangent_in.length_squared() < 0.001f) {
                contour.push_back(p3);
            } else {
                flatten_bezier(p0, p1, p2, p3, contour);
            }
        }

        if (path.closed && path.points.size() > 1) {
            const auto& pt_a = path.points.back();
            const auto& pt_b = path.points.front();
            math::Vector2 p0 = pt_a.position;
            math::Vector2 p1 = p0 + pt_a.tangent_out;
            math::Vector2 p3 = pt_b.position;
            math::Vector2 p2 = p3 + pt_b.tangent_in;

            if (!(pt_a.tangent_out.length_squared() < 0.001f && pt_b.tangent_in.length_squared() < 0.001f)) {
                flatten_bezier(p0, p1, p2, p3, contour);
            }
        }

        if (contour.size() >= 2U && (contour.front() - contour.back()).length_squared() < 0.001f) {
            contour.pop_back();
        }
        return contour;
    }

    MeshAsset::SubMesh extrude_compound_polygons(
        const std::vector<std::vector<math::Vector2>>& rings,
        float depth) {
        MeshAsset::SubMesh mesh;
        if (rings.empty()) {
            return mesh;
        }

        PolygonType polygon;
        polygon.reserve(rings.size());
        for (const auto& ring : rings) {
            if (ring.size() < 3U) {
                continue;
            }
            PolygonType::value_type polygon_ring;
            polygon_ring.reserve(ring.size());
            for (const auto& p : ring) {
                polygon_ring.push_back({p.x, p.y});
            }
            polygon.push_back(std::move(polygon_ring));
        }

        if (polygon.empty()) {
            return mesh;
        }

        const std::vector<unsigned int> cap_indices = mapbox::earcut<unsigned int>(polygon);
        const float z_front = depth * 0.5f;
        const float z_back = -depth * 0.5f;

        for (const auto& ring : polygon) {
            for (const auto& p : ring) {
                MeshAsset::Vertex v;
                v.position = {p[0], p[1], z_front};
                v.normal = {0.0f, 0.0f, 1.0f};
                v.uv = {p[0] / 100.0f, p[1] / 100.0f};
                mesh.vertices.push_back(v);
            }
        }

        for (std::size_t i = 0; i + 2 < cap_indices.size(); i += 3) {
            mesh.indices.push_back(cap_indices[i + 0]);
            mesh.indices.push_back(cap_indices[i + 1]);
            mesh.indices.push_back(cap_indices[i + 2]);
        }

        if (std::abs(depth) > 0.001f) {
            const unsigned int back_offset = static_cast<unsigned int>(mesh.vertices.size());
            for (const auto& ring : polygon) {
                for (const auto& p : ring) {
                    MeshAsset::Vertex v;
                    v.position = {p[0], p[1], z_back};
                    v.normal = {0.0f, 0.0f, -1.0f};
                    v.uv = {p[0] / 100.0f, p[1] / 100.0f};
                    mesh.vertices.push_back(v);
                }
            }

            std::size_t running_offset = 0;
            for (const auto& ring : polygon) {
                const std::size_t n = ring.size();
                for (std::size_t i = 0; i < n; ++i) {
                    const std::size_t j = (i + 1) % n;
                    const math::Vector2 a{ring[i][0], ring[i][1]};
                    const math::Vector2 b{ring[j][0], ring[j][1]};
                    const math::Vector2 edge = b - a;
                    const math::Vector2 outward = math::Vector2{edge.y, -edge.x}.normalized();

                    MeshAsset::Vertex sv0; sv0.position = {a.x, a.y, z_front}; sv0.normal = {outward.x, outward.y, 0.0f}; sv0.uv = {0.0f, 0.0f};
                    MeshAsset::Vertex sv1; sv1.position = {b.x, b.y, z_front}; sv1.normal = {outward.x, outward.y, 0.0f}; sv1.uv = {1.0f, 0.0f};
                    MeshAsset::Vertex sv2; sv2.position = {a.x, a.y, z_back};  sv2.normal = {outward.x, outward.y, 0.0f}; sv2.uv = {0.0f, 1.0f};
                    MeshAsset::Vertex sv3; sv3.position = {b.x, b.y, z_back};  sv3.normal = {outward.x, outward.y, 0.0f}; sv3.uv = {1.0f, 1.0f};

                    const unsigned int wall_offset = static_cast<unsigned int>(mesh.vertices.size());
                    mesh.vertices.push_back(sv0);
                    mesh.vertices.push_back(sv1);
                    mesh.vertices.push_back(sv2);
                    mesh.vertices.push_back(sv3);

                    mesh.indices.push_back(wall_offset + 0);
                    mesh.indices.push_back(wall_offset + 2);
                    mesh.indices.push_back(wall_offset + 1);

                    mesh.indices.push_back(wall_offset + 1);
                    mesh.indices.push_back(wall_offset + 2);
                    mesh.indices.push_back(wall_offset + 3);
                }
                running_offset += n;
            }

            for (std::size_t i = 0; i + 2 < cap_indices.size(); i += 3) {
                mesh.indices.push_back(back_offset + cap_indices[i + 0]);
                mesh.indices.push_back(back_offset + cap_indices[i + 2]);
                mesh.indices.push_back(back_offset + cap_indices[i + 1]);
            }
        }

        mesh.material.base_color_factor = {1.0f, 1.0f, 1.0f};
        mesh.transform = math::Matrix4x4::identity();
        return mesh;
    }
}

MeshAsset::SubMesh Extruder::extrude_shape(
    const scene::EvaluatedShapePath& path,
    float depth,
    float /*bevel_size*/)
{
    return extrude_compound_polygons({flatten_shape_path(path)}, depth);
}

MeshAsset::SubMesh Extruder::extrude_shape(
    const std::vector<scene::EvaluatedShapePath>& paths,
    float depth,
    float /*bevel_size*/)
{
    struct RingInfo {
        std::vector<math::Vector2> contour;
        float area{0.0f};
        int depth{0};
    };

    std::vector<RingInfo> rings;
    rings.reserve(paths.size());
    for (const auto& path : paths) {
        auto contour = flatten_shape_path(path);
        if (contour.size() < 3U) {
            continue;
        }
        rings.push_back({std::move(contour), 0.0f, 0});
    }

    if (rings.empty()) {
        return {};
    }

    for (auto& ring : rings) {
        ring.area = signed_area(ring.contour);
    }

    for (std::size_t i = 0; i < rings.size(); ++i) {
        const auto sample = contour_centroid(rings[i].contour);
        int nesting_depth = 0;
        for (std::size_t j = 0; j < rings.size(); ++j) {
            if (i == j) {
                continue;
            }
            if (std::abs(rings[j].area) <= std::abs(rings[i].area)) {
                continue;
            }
            if (point_in_polygon(sample, rings[j].contour)) {
                ++nesting_depth;
            }
        }
        rings[i].depth = nesting_depth;
    }

    std::vector<MeshAsset::SubMesh> components;
    for (std::size_t i = 0; i < rings.size(); ++i) {
        if (rings[i].depth != 0) {
            continue;
        }

        std::vector<std::vector<math::Vector2>> component_rings;
        component_rings.push_back(rings[i].contour);

        for (std::size_t j = 0; j < rings.size(); ++j) {
            if (i == j || rings[j].depth != 1) {
                continue;
            }
            if (point_in_polygon(contour_centroid(rings[j].contour), rings[i].contour)) {
                component_rings.push_back(rings[j].contour);
            }
        }

        components.push_back(extrude_compound_polygons(component_rings, depth));
    }

    if (components.empty()) {
        return {};
    }

    if (components.size() == 1) {
        return std::move(components.front());
    }

    MeshAsset::SubMesh merged;
    for (auto& component : components) {
        const unsigned int vertex_offset = static_cast<unsigned int>(merged.vertices.size());
        merged.vertices.insert(merged.vertices.end(), component.vertices.begin(), component.vertices.end());
        for (auto index : component.indices) {
            merged.indices.push_back(index + vertex_offset);
        }
    }
    merged.material.base_color_factor = {1.0f, 1.0f, 1.0f};
    merged.transform = math::Matrix4x4::identity();
    return merged;
}

} // namespace tachyon::media
