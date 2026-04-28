#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/deform/mesh_deform.h"
#include <memory>

namespace tachyon::renderer2d {

/**
 * @brief Apply mesh deformation to a layer surface.
 * 
 * Uses the layer's mesh_deform data to deform the surface.
 * If mesh_deform is not enabled or not available, returns the original surface.
 * 
 * @param surface The source surface to deform.
 * @param mesh The DeformMesh to use for deformation.
 * @param width The width of the output.
 * @param height The height of the output.
 * @return A new surface with the deformation applied, or the original if no deformation.
 */
std::shared_ptr<SurfaceRGBA> apply_mesh_deform(
    const std::shared_ptr<SurfaceRGBA>& surface,
    const DeformMesh& mesh,
    std::int64_t width,
    std::int64_t height);

} // namespace tachyon::renderer2d
