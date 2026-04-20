#pragma once

#include "tachyon/renderer2d/path/contour/contour_processing.h"
#include "tachyon/renderer2d/path_rasterizer.h"

namespace tachyon::renderer2d {

class CoverageCalculator {
public:
    static constexpr int kCoverageSamples = 4;

    static float fill_coverage(const std::vector<Contour>& contours, int x, int y, WindingRule winding);
    static float stroke_coverage(const std::vector<Contour>& contours, int x, int y, float radius, const StrokePathStyle& style);

private:
    static bool point_inside_even_odd(const std::vector<Contour>& contours, const math::Vector2& point);
    static bool point_inside_non_zero(const std::vector<Contour>& contours, const math::Vector2& point);
    static int winding_number(const std::vector<Contour>& contours, const math::Vector2& point);
};

} // namespace tachyon::renderer2d
