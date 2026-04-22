#pragma once

#include <vector>

namespace tachyon::shapes {

struct Point2D {
    double x{0.0};
    double y{0.0};

    Point2D operator+(const Point2D& other) const { return {x + other.x, y + other.y}; }
    Point2D operator-(const Point2D& other) const { return {x - other.x, y - other.y}; }
    Point2D operator*(double scalar) const { return {x * scalar, y * scalar}; }
};

/**
 * @brief A vertex on a bezier path, storing its position and tangents.
 */
struct PathVertex {
    Point2D point;
    Point2D in_tangent;  // Relative to point
    Point2D out_tangent; // Relative to point
};

/**
 * @brief A continuous sequence of cubic bezier segments.
 */
struct ShapeSubpath {
    std::vector<PathVertex> vertices;
    bool closed{false};

    [[nodiscard]] bool empty() const { return vertices.empty(); }
    [[nodiscard]] std::size_t size() const { return vertices.size(); }
};

/**
 * @brief A full shape path, consisting of multiple subpaths (e.g. for holes).
 */
struct ShapePath {
    std::vector<ShapeSubpath> subpaths;

    [[nodiscard]] bool empty() const { return subpaths.empty(); }
};

} // namespace tachyon::shapes
