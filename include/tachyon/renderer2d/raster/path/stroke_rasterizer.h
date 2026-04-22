#pragma once

#include "tachyon/renderer2d/raster/path/path_types.h"
#include "tachyon/renderer2d/raster/path/path_flattener.h"

namespace tachyon::renderer2d {

void rasterize_stroke_polygon(SurfaceRGBA& surface, const std::vector<Contour>& contours, const StrokePathStyle& style);

} // namespace tachyon::renderer2d
