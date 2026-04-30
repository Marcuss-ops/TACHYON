#pragma once

#include "tachyon/core/math/vector2.h"

#include <vector>

namespace tachyon::shapes {

using Point2D = math::Vector2;

struct PathVertex {
    Point2D point;
    Point2D in_tangent{0.0f, 0.0f};
    Point2D out_tangent{0.0f, 0.0f};
    Point2D position;
    Point2D tangent_in;
    Point2D tangent_out;
};

struct ShapeSubpath {
    std::vector<PathVertex> vertices;
    bool closed{false};
};

struct ShapePathSpec {
    std::vector<PathVertex> points;
    bool closed{false};
    std::vector<ShapeSubpath> subpaths;

    [[nodiscard]] bool empty() const noexcept {
        return points.empty() && subpaths.empty();
    }
};

using ShapePath = ShapePathSpec;

} // namespace tachyon::shapes

namespace tachyon {
using Point2D = shapes::Point2D;
using PathVertex = shapes::PathVertex;
using ShapeSubpath = shapes::ShapeSubpath;
using ShapePathSpec = shapes::ShapePathSpec;
using ShapePath = shapes::ShapePath;
} // namespace tachyon
