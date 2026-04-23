#pragma once

#include "tachyon/renderer2d/raster/path/path_types.h"
#include <vector>

namespace tachyon::renderer2d {

struct ContourPoint {
    math::Vector2 point;
    float feather_inner{0.0f};
    float feather_outer{0.0f};
};

struct Contour {
    std::vector<ContourPoint> points;
};

void flatten_cubic(
    const math::Vector2& p0,
    const math::Vector2& p1,
    const math::Vector2& p2,
    const math::Vector2& p3,
    float feather_inner_start,
    float feather_outer_start,
    float feather_inner_end,
    float feather_outer_end,
    std::vector<ContourPoint>& out,
    float tolerance,
    std::uint32_t depth = 0U);

std::vector<Contour> build_contours(const PathGeometry& path);

float segment_length(const math::Vector2& p0, const math::Vector2& p1, const math::Vector2& p2, const math::Vector2& p3, PathVerb verb);

} // namespace tachyon::renderer2d
