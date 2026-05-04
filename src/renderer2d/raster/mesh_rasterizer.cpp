#include "tachyon/renderer2d/raster/mesh_rasterizer.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace tachyon::renderer2d {

void MeshRasterizer::barycentric(float px, float py,
                                   float ax, float ay,
                                   float bx, float by,
                                   float cx, float cy,
                                   float& u, float& v, float& w) {
    float v0x = bx - ax;
    float v0y = by - ay;
    float v1x = cx - ax;
    float v1y = cy - ay;
    float v2x = px - ax;
    float v2y = py - ay;
    
    float denom = v0x * v1y - v1x * v0y;
    if (std::abs(denom) < 1e-10f) {
        u = v = w = 0.0f;
        return;
    }
    
    float inv_denom = 1.0f / denom;
    v = (v2x * v1y - v1x * v2y) * inv_denom;
    w = (v0x * v2y - v2x * v0y) * inv_denom;
    u = 1.0f - v - w;
}

void MeshRasterizer::rasterize_triangle(const DeformMesh::Vertex& v0,
                                         const DeformMesh::Vertex& v1,
                                         const DeformMesh::Vertex& v2,
                                         const SurfaceRGBA& source,
                                         std::uint8_t* output_rgba,
                                         std::int64_t output_width,
                                         std::int64_t output_height) {
    // Compute bounding box
    float min_x = std::min({v0.position.x, v1.position.x, v2.position.x});
    float max_x = std::max({v0.position.x, v1.position.x, v2.position.x});
    float min_y = std::min({v0.position.y, v1.position.y, v2.position.y});
    float max_y = std::max({v0.position.y, v1.position.y, v2.position.y});
    
    // Clamp to output bounds
    int start_x = std::max(0, static_cast<int>(std::floor(min_x)));
    int end_x = std::min(static_cast<int>(output_width) - 1, static_cast<int>(std::ceil(max_x)));
    int start_y = std::max(0, static_cast<int>(std::floor(min_y)));
    int end_y = std::min(static_cast<int>(output_height) - 1, static_cast<int>(std::ceil(max_y)));
    
    if (start_x > end_x || start_y > end_y) return;
    
    const float src_w = static_cast<float>(source.width());
    const float src_h = static_cast<float>(source.height());
    
    for (int y = start_y; y <= end_y; ++y) {
        for (int x = start_x; x <= end_x; ++x) {
            float px = static_cast<float>(x) + 0.5f;
            float py = static_cast<float>(y) + 0.5f;
            
            float u, v, w;
            barycentric(px, py,
                       v0.position.x, v0.position.y,
                       v1.position.x, v1.position.y,
                       v2.position.x, v2.position.y,
                       u, v, w);
            
            // Check if point is inside triangle (with small epsilon)
            if (u < -1e-4f || v < -1e-4f || w < -1e-4f) continue;
            
            // Interpolate UV coordinates
            float interp_u = u * v0.uv.x + v * v1.uv.x + w * v2.uv.x;
            float interp_v = u * v0.uv.y + v * v1.uv.y + w * v2.uv.y;
            
            // Sample source texture using bilinear interpolation
            float src_x = interp_u * src_w - 0.5f;
            float src_y = interp_v * src_h - 0.5f;
            
            // Manual bilinear sampling
            int x0 = static_cast<int>(std::floor(src_x));
            int y0 = static_cast<int>(std::floor(src_y));
            int x1 = x0 + 1;
            int y1 = y0 + 1;
            
            float fx = src_x - static_cast<float>(x0);
            float fy = src_y - static_cast<float>(y0);
            
            // Clamp coordinates
            x0 = std::clamp(x0, 0, static_cast<int>(source.width()) - 1);
            x1 = std::clamp(x1, 0, static_cast<int>(source.width()) - 1);
            y0 = std::clamp(y0, 0, static_cast<int>(source.height()) - 1);
            y1 = std::clamp(y1, 0, static_cast<int>(source.height()) - 1);
            
            // Sample 4 corners
            Color c00 = source.get_pixel(static_cast<uint32_t>(x0), static_cast<uint32_t>(y0));
            Color c10 = source.get_pixel(static_cast<uint32_t>(x1), static_cast<uint32_t>(y0));
            Color c01 = source.get_pixel(static_cast<uint32_t>(x0), static_cast<uint32_t>(y1));
            Color c11 = source.get_pixel(static_cast<uint32_t>(x1), static_cast<uint32_t>(y1));
            
            // Bilinear interpolate
            Color sampled;
            sampled.r = (1.0f - fy) * ((1.0f - fx) * c00.r + fx * c10.r) + fy * ((1.0f - fx) * c01.r + fx * c11.r);
            sampled.g = (1.0f - fy) * ((1.0f - fx) * c00.g + fx * c10.g) + fy * ((1.0f - fx) * c01.g + fx * c11.g);
            sampled.b = (1.0f - fy) * ((1.0f - fx) * c00.b + fx * c10.b) + fy * ((1.0f - fx) * c01.b + fx * c11.b);
            sampled.a = (1.0f - fy) * ((1.0f - fx) * c00.a + fx * c10.a) + fy * ((1.0f - fx) * c01.a + fx * c11.a);
            
            // Write to output
            std::size_t idx = (y * output_width + x) * 4;
            output_rgba[idx + 0] = static_cast<std::uint8_t>(sampled.r * 255.0f);
            output_rgba[idx + 1] = static_cast<std::uint8_t>(sampled.g * 255.0f);
            output_rgba[idx + 2] = static_cast<std::uint8_t>(sampled.b * 255.0f);
            output_rgba[idx + 3] = static_cast<std::uint8_t>(sampled.a * 255.0f);
        }
    }
}

void MeshRasterizer::rasterize(const DeformMesh& mesh,
                                const SurfaceRGBA& source,
                                std::uint8_t* output_rgba,
                                std::int64_t output_width,
                                std::int64_t output_height) {
    if (!output_rgba || output_width <= 0 || output_height <= 0) return;
    
    // Clear output to transparent
    std::size_t pixel_count = static_cast<std::size_t>(output_width * output_height);
    std::memset(output_rgba, 0, pixel_count * 4);
    
    // Rasterize each triangle
    for (const auto& tri : mesh.triangles) {
        if (tri.indices[0] >= mesh.vertices.size() ||
            tri.indices[1] >= mesh.vertices.size() ||
            tri.indices[2] >= mesh.vertices.size()) {
            continue;
        }
        
        const auto& v0 = mesh.vertices[tri.indices[0]];
        const auto& v1 = mesh.vertices[tri.indices[1]];
        const auto& v2 = mesh.vertices[tri.indices[2]];
        
        rasterize_triangle(v0, v1, v2, source, output_rgba, output_width, output_height);
    }
}

} // namespace tachyon::renderer2d
