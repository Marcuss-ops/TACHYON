#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/core/spec/schema/shapes/shape_spec.h"
#include <memory>

namespace tachyon::renderer2d {

/**
 * @brief Rasterize a shape using Signed Distance Field rendering.
 * This provides high-quality anti-aliased edges at any scale.
 */
class SDFShapeRasterizer {
public:
    static Framebuffer rasterize(const ShapeSpec& shape_spec, int width, int height);
};

} // namespace tachyon::renderer2d
