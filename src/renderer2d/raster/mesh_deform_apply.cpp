#include "tachyon/renderer2d/raster/mesh_deform_apply.h"
#include "tachyon/renderer2d/raster/mesh_rasterizer.h"
#include <vector>
#include <cstdint>

namespace tachyon::renderer2d {

std::shared_ptr<SurfaceRGBA> apply_mesh_deform(
    const std::shared_ptr<SurfaceRGBA>& surface,
    const DeformMesh& mesh,
    std::int64_t width,
    std::int64_t height) {
    
    if (!surface || width <= 0 || height <= 0) {
        return surface;
    }
    
    auto output = std::make_shared<SurfaceRGBA>(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
    if (!output) {
        return surface;
    }
    
    // Allocate buffer for output
    std::vector<std::uint8_t> output_rgba(static_cast<std::size_t>(width * height * 4));
    
    // Rasterize the deformed mesh
    MeshRasterizer::rasterize(mesh, *surface, output_rgba.data(), width, height);
    
    // Copy to SurfaceRGBA
    for (std::int64_t y = 0; y < height; ++y) {
        for (std::int64_t x = 0; x < width; ++x) {
            std::size_t idx = static_cast<std::size_t>((y * width + x) * 4);
            output->set_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), {
                output_rgba[idx + 0] / 255.0f,
                output_rgba[idx + 1] / 255.0f,
                output_rgba[idx + 2] / 255.0f,
                output_rgba[idx + 3] / 255.0f
            });
        }
    }
    
    return output;
}

} // namespace tachyon::renderer2d
