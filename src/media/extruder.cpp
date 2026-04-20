#include "tachyon/media/extruder.h"
#include <mapbox/earcut.hpp>
#include <array>

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
}

MeshAsset::SubMesh Extruder::extrude_shape(
    const scene::EvaluatedShapePath& path, 
    float depth, 
    float bevel_size) 
{
    MeshAsset::SubMesh mesh;
    if (path.points.empty()) return mesh;

    // 1. Flatten the shape path
    std::vector<math::Vector2> contour;
    if (!path.points.empty()) {
        contour.push_back(path.points[0].position);
        
        for (std::size_t i = 1; i < path.points.size(); ++i) {
            const auto& pt_a = path.points[i - 1];
            const auto& pt_b = path.points[i];
            
            math::Vector2 p0 = pt_a.position;
            math::Vector2 p1 = p0 + pt_a.tangent_out;
            math::Vector2 p3 = pt_b.position;
            math::Vector2 p2 = p3 + pt_b.tangent_in;

            if (pt_a.tangent_out.length_squared() < 0.001f && pt_b.tangent_in.length_squared() < 0.001f) {
                // Straight line
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

            if (pt_a.tangent_out.length_squared() < 0.001f && pt_b.tangent_in.length_squared() < 0.001f) {
                // Skip if closing point overlaps perfectly
            } else {
                flatten_bezier(p0, p1, p2, p3, contour);
            }
        }
    }

    if (contour.size() < 3) return mesh;

    // Remove overlapping exact matching last point (common in closing loops)
    if ((contour.front() - contour.back()).length_squared() < 0.001f) {
        contour.pop_back();
    }

    // 2. Prepare for earcut
    PolygonType polygon;
    polygon.emplace_back(); // Outer ring
    for (const auto& p : contour) {
        polygon.back().push_back({p.x, p.y}); // Inverted Y is standard for AE but handle correctly downstream
    }

    std::vector<unsigned int> cap_indices = mapbox::earcut<unsigned int>(polygon);

    // 3. Generate Geometry
    float z_front = depth * 0.5f;
    float z_back = -depth * 0.5f;

    // Insert front vertices
    unsigned int v_offset_front = static_cast<unsigned int>(mesh.vertices.size());
    for (const auto& p : contour) {
        MeshAsset::Vertex v;
        v.position = {p.x, p.y, z_front};
        v.normal = {0, 0, 1}; // Face normal
        v.uv = {p.x / 100.0f, p.y / 100.0f}; // Simple planar map
        mesh.vertices.push_back(v);
    }
    
    // Insert back vertices
    unsigned int v_offset_back = static_cast<unsigned int>(mesh.vertices.size());
    for (const auto& p : contour) {
        MeshAsset::Vertex v;
        v.position = {p.x, p.y, z_back};
        v.normal = {0, 0, -1}; // Back face normal
        v.uv = {p.x / 100.0f, p.y / 100.0f};
        mesh.vertices.push_back(v);
    }

    // Front Cap Indices
    for (std::size_t i = 0; i < cap_indices.size(); i += 3) {
        mesh.indices.push_back(cap_indices[i] + v_offset_front);
        mesh.indices.push_back(cap_indices[i+1] + v_offset_front);
        mesh.indices.push_back(cap_indices[i+2] + v_offset_front);
    }

    // Back Cap Indices (reversed winding)
    for (std::size_t i = 0; i < cap_indices.size(); i += 3) {
        mesh.indices.push_back(cap_indices[i] + v_offset_back);
        mesh.indices.push_back(cap_indices[i+2] + v_offset_back);
        mesh.indices.push_back(cap_indices[i+1] + v_offset_back);
    }

    // 4. Generate Side Walls
    std::size_t n = contour.size();
    for (std::size_t i = 0; i < n; ++i) {
        std::size_t j = (i + 1) % n;

        math::Vector3 p0 = {contour[i].x, contour[i].y, z_front};
        math::Vector3 p1 = {contour[j].x, contour[j].y, z_front};
        math::Vector3 p2 = {contour[i].x, contour[i].y, z_back};
        math::Vector3 p3 = {contour[j].x, contour[j].y, z_back};

        math::Vector3 v1 = p1 - p0;
        math::Vector3 v2 = p2 - p0;
        math::Vector3 side_normal = math::Vector3::cross(v1, v2).normalized();

        unsigned int b_v0 = static_cast<unsigned int>(mesh.vertices.size());
        
        // Vertices 0 and 1 (Front edge)
        MeshAsset::Vertex sv0; sv0.position = p0; sv0.normal = side_normal; sv0.uv = {0,0};
        MeshAsset::Vertex sv1; sv1.position = p1; sv1.normal = side_normal; sv1.uv = {1,0};
        // Vertices 2 and 3 (Back edge)
        MeshAsset::Vertex sv2; sv2.position = p2; sv2.normal = side_normal; sv2.uv = {0,1};
        MeshAsset::Vertex sv3; sv3.position = p3; sv3.normal = side_normal; sv3.uv = {1,1};

        mesh.vertices.push_back(sv0);
        mesh.vertices.push_back(sv1);
        mesh.vertices.push_back(sv2);
        mesh.vertices.push_back(sv3);

        mesh.indices.push_back(b_v0);
        mesh.indices.push_back(b_v0 + 2);
        mesh.indices.push_back(b_v0 + 1);

        mesh.indices.push_back(b_v0 + 1);
        mesh.indices.push_back(b_v0 + 2);
        mesh.indices.push_back(b_v0 + 3);
    }

    // Assign default white PBR so it responds nicely to lighting
    mesh.material.base_color_factor = {1.0f, 1.0f, 1.0f};
    mesh.transform = math::Matrix4x4::identity();

    return mesh;
}

} // namespace tachyon::media
