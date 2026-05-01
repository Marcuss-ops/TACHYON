#pragma once

#include "tachyon/core/shapes/shape_path.h"
#include <vector>
#include <cmath>

// Clipper2 for Boolean Ops
#include "clipper2/clipper.h"

namespace tachyon::shapes {

/**
 * @brief Transforms applied to each instance in a repeater.
 */
struct RepeaterTransform {
    Point2D anchor_point;
    Point2D position;
    Point2D scale{1.0, 1.0};
    double rotation{0.0};
    double start_opacity{1.0};
    double end_opacity{1.0};
};

class ShapeModifiers {
public:
    /**
     * @brief Trim Paths modifier.
     * Uses parametric interpolation to trim bezier paths.
     */
    /**
     * @brief Precise De Casteljau split for a cubic bezier segment.
     * Returns two segments (left and right) at parameter t [0, 1].
     */
    static void split_cubic_bezier(const PathVertex& v1, const PathVertex& v2, double t, PathVertex& left_end, PathVertex& right_start);

    /**
     * @brief Trim Paths modifier.
     * Uses parametric interpolation to trim bezier paths.
     */
    static ShapePath trim_paths(const ShapePath& path, double start, double end, double offset);

    /**
     * @brief Repeater modifier.
     * Applies cumulative affine transformations.
     */
    static ShapePath repeater(const ShapePath& path, int copies, const RepeaterTransform& transform);

    /**
     * @brief Offset Paths modifier.
     */
    static ShapePath offset_path(const ShapePath& path, double amount);

    /**
     * @brief Merge Paths modifier using Clipper2.
     * mode: 0=Add (Union), 1=Subtract (Difference), 2=Intersect, 3=Exclude (Xor)
     */
    static ShapePath merge_paths(const ShapePath& a, const ShapePath& b, int mode);

    /**
     * @brief Twist modifier.
     */
    static ShapePath twist(const ShapePath& path, double angle, const Point2D& center) {
        if (path.empty() || angle == 0.0) return path;

        ShapePath result;
        for (const auto& subpath : path.subpaths) {
            ShapeSubpath twisted = subpath;
            for (auto& v : twisted.vertices) {
                double dx = v.point.x - center.x;
                double dy = v.point.y - center.y;
                double dist = std::sqrt(dx * dx + dy * dy);
                
                // Twist amount falls off with distance
                double twist_rad = (angle * 3.141592653589793 / 180.0) / (dist + 1.0);
                
                double cos_a = std::cos(twist_rad);
                double sin_a = std::sin(twist_rad);
                
                v.point.x = static_cast<float>(center.x + dx * cos_a - dy * sin_a);
                v.point.y = static_cast<float>(center.y + dx * sin_a + dy * cos_a);
            }
            result.subpaths.push_back(twisted);
        }
        return result;
    }
};

} // namespace tachyon::shapes
