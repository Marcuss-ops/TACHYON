#pragma once

#include "tachyon/core/shapes/shape_path.h"
#include "tachyon/core/math/utils/noise.h"
#include <vector>
#include <cstdint>

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
    static ShapePath twist(const ShapePath& path, double angle, const Point2D& center);

    /**
     * @brief Oscillator modifier.
     */
    static ShapePath oscillator(const ShapePath& path, double frequency, double amplitude, double phase, double time, int axis = 2);

    /**
     * @brief Noise Deform modifier.
     */
    static ShapePath noise_deform(const ShapePath& path, double noise_scale, double amplitude, double time, uint64_t seed = 0);
};

} // namespace tachyon::shapes
