#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include <string>

namespace tachyon::renderer3d {

/**
 * @brief Represents the multi-pass output of the 3D RayTracer.
 */
struct AOVBuffer {
    std::uint32_t width{0};
    std::uint32_t height{0};

    // Primary Beauty Pass (HDR RGBA)
    std::vector<float> beauty_rgba;

    // Standard AOVs
    std::vector<float> depth_z;          // Z-depth from camera (1 channel)
    std::vector<float> normal_xyz;       // World-space normals (3 channels)
    std::vector<float> motion_vector_xy; // Screen-space velocity (2 channels)
    
    // IDs for masking in compositing
    std::vector<std::uint32_t> object_id;
    std::vector<std::uint32_t> material_id;

    void allocate(std::uint32_t w, std::uint32_t h) {
        width = w;
        height = h;
        const std::size_t num_pixels = static_cast<std::size_t>(w) * h;
        beauty_rgba.assign(num_pixels * 4, 0.0f);
        depth_z.assign(num_pixels, 0.0f);
        normal_xyz.assign(num_pixels * 3, 0.0f);
        motion_vector_xy.assign(num_pixels * 2, 0.0f);
        object_id.assign(num_pixels, 0);
        material_id.assign(num_pixels, 0);
    }
};

} // namespace tachyon::renderer3d
