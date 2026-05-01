#pragma once

#include "tachyon/renderer2d/raster/path_rasterizer.h"
#include "tachyon/core/math/vector2.h"
#include <vector>

namespace tachyon::renderer2d {

class ContourProcessor {
public:
    static std::vector<Contour> build_contours(const PathGeometry& path, float tolerance = 0.35f);
};

} // namespace tachyon::renderer2d
