#pragma once

#include "tachyon/renderer2d/raster/path/path_types.h"
#include "tachyon/renderer2d/raster/path/path_flattener.h"
#include "tachyon/renderer2d/raster/path/fill_rasterizer.h"
#include "tachyon/renderer2d/raster/path/stroke_rasterizer.h"
#include "tachyon/renderer2d/raster/path/path_trimmer.h"

namespace tachyon::renderer2d {

class PathRasterizer {
public:
    static void fill(SurfaceRGBA& surface, const PathGeometry& path, const FillPathStyle& style) {
        rasterize_fill_polygon(surface, build_contours(path), style);
    }
    
    static void stroke(SurfaceRGBA& surface, const PathGeometry& path, const StrokePathStyle& style) {
        rasterize_stroke_polygon(surface, build_contours(path), style);
    }

    static PathGeometry trim(const PathGeometry& path, float start, float end, float offset) {
        return trim_path(path, start, end, offset);
    }
};

} // namespace tachyon::renderer2d
