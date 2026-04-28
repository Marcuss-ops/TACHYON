#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/deform/mesh_deform.h"
#include <cstdint>

namespace tachyon::renderer2d {

/**
 * @brief Rasterizes a deformed mesh to a surface.
 * 
 * Uses texture mapping with bilinear interpolation.
 * The mesh's UV coordinates are used to sample from the source surface.
 */
class MeshRasterizer {
public:
    /**
     * @brief Rasterize a deformed mesh to an output surface.
     * 
     * @param mesh The deformed mesh with vertices and triangles.
     * @param source The source surface to sample texture from.
     * @param output The output surface to render to.
     * @param output_width Width of the output surface.
     * @param output_height Height of the output surface.
     */
    static void rasterize(const DeformMesh& mesh,
                         const SurfaceRGBA& source,
                         std::uint8_t* output_rgba,
                         std::int64_t output_width,
                         std::int64_t output_height);
    
private:
    /**
     * @brief Rasterize a single triangle.
     */
    static void rasterize_triangle(const DeformMesh::Vertex& v0,
                                   const DeformMesh::Vertex& v1,
                                   const DeformMesh::Vertex& v2,
                                   const SurfaceRGBA& source,
                                   std::uint8_t* output_rgba,
                                   std::int64_t output_width,
                                   std::int64_t output_height);
    
    /**
     * @brief Compute barycentric coordinates for a point in a triangle.
     */
    static void barycentric(float px, float py,
                           float ax, float ay,
                           float bx, float by,
                           float cx, float cy,
                           float& u, float& v, float& w);
};

} // namespace tachyon::renderer2d
