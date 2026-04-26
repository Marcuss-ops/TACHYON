#pragma once

#include "tachyon/core/math/vector2.h"

#include <vector>
#include <cmath>
#include <algorithm>
#include <memory>

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

    // Shape morphing: target points to interpolate towards (must be same size as points)
    std::vector<PathVertex> morph_target_points;
    double morph_progress{0.0}; // 0.0 = original, 1.0 = fully morphed

    [[nodiscard]] bool empty() const noexcept {
        return points.empty() && subpaths.empty();
    }

    /**
     * Apply shape morphing interpolation using stored morph_target_points.
     * @return Interpolated shape, or original if no morphing needed
     */
    [[nodiscard]] ShapePathSpec apply_morph() const {
        ShapePathSpec result = *this;
        if (morph_target_points.empty() || morph_progress <= 0.0) return result;
        if (morph_progress >= 1.0) {
            result.points = morph_target_points;
            return result;
        }

        double t = std::clamp(morph_progress, 0.0, 1.0);
        size_t num_points = std::min(points.size(), morph_target_points.size());
        if (num_points == 0) return result;

        result.points.clear();
        result.points.reserve(num_points);
        for (size_t i = 0; i < num_points; ++i) {
            PathVertex v;
            v.point.x = points[i].point.x + (morph_target_points[i].point.x - points[i].point.x) * static_cast<float>(t);
            v.point.y = points[i].point.y + (morph_target_points[i].point.y - points[i].point.y) * static_cast<float>(t);
            v.position = v.point;
            result.points.push_back(v);
        }
        return result;
    }

    /**
     * Resample the path to have uniformly spaced points using arc-length parameterization.
     * @param num_points Number of points in the output
     * @return Vector of uniformly spaced points
     */
    [[nodiscard]] std::vector<Point2D> resample_to_uniform(int num_points) const {
        std::vector<Point2D> result;
        if (num_points <= 0) return result;

        const auto& vertices = subpaths.empty() ? points : subpaths[0].vertices;
        if (vertices.empty()) return result;

        // Compute arc-length table
        std::vector<double> arc_lengths;
        arc_lengths.push_back(0.0);
        for (size_t i = 1; i < vertices.size(); ++i) {
            double dx = vertices[i].point.x - vertices[i-1].point.x;
            double dy = vertices[i].point.y - vertices[i-1].point.y;
            arc_lengths.push_back(arc_lengths.back() + std::sqrt(dx*dx + dy*dy));
        }
        double total_length = arc_lengths.back();

        if (total_length <= 0.0) {
            // Degenerate case: all points are the same
            result.resize(num_points, vertices[0].point);
            return result;
        }

        result.reserve(num_points);
        for (int i = 0; i < num_points; ++i) {
            double target_dist = (static_cast<double>(i) / static_cast<double>(num_points - 1)) * total_length;
            // Find segment containing target_dist
            size_t seg = 0;
            for (size_t j = 1; j < arc_lengths.size(); ++j) {
                if (arc_lengths[j] >= target_dist) {
                    seg = j - 1;
                    break;
                }
            }
            if (seg >= vertices.size() - 1) {
                result.push_back(vertices.back().point);
                continue;
            }
            double seg_len = arc_lengths[seg + 1] - arc_lengths[seg];
            double frac = (seg_len > 0.0) ? (target_dist - arc_lengths[seg]) / seg_len : 0.0;
            frac = std::clamp(frac, 0.0, 1.0);

            Point2D p;
            p.x = vertices[seg].point.x + static_cast<float>((vertices[seg + 1].point.x - vertices[seg].point.x) * frac);
            p.y = vertices[seg].point.y + static_cast<float>((vertices[seg + 1].point.y - vertices[seg].point.y) * frac);
            result.push_back(p);
        }
        return result;
    }

    /**
     * Sample a point on the path at progress t in [0,1].
     * Uses arc-length parameterization for uniform speed along the path.
     * @param t Progress along path (0.0 = start, 1.0 = end)
     * @return The sampled point
     */
    [[nodiscard]] Point2D sample_point(double t) const {
        if (empty()) return {0.0f, 0.0f};

        t = std::clamp(t, 0.0, 1.0);

        // Build arc-length table if not cached (simplified version)
        const auto& vertices = subpaths.empty() ? points : subpaths[0].vertices;
        if (vertices.empty()) return {0.0f, 0.0f};

        // For now, simple linear interpolation between vertices
        // A full implementation would use arc-length parameterization
        float total_segments = static_cast<float>(vertices.size() - 1);
        float segment_float = static_cast<float>(t) * total_segments;
        int segment = static_cast<int>(segment_float);
        float frac = segment_float - static_cast<float>(segment);

        if (segment >= static_cast<int>(vertices.size()) - 1) {
            return vertices.back().point;
        }

        const auto& v0 = vertices[segment];
        const auto& v1 = vertices[segment + 1];

        // Simple linear interpolation (can be enhanced with Bezier)
        return {
            v0.point.x + static_cast<float>(v1.point.x - v0.point.x) * static_cast<float>(frac),
            v0.point.y + static_cast<float>(v1.point.y - v0.point.y) * static_cast<float>(frac)
        };
    }

    /**
     * Sample the tangent angle (in radians) at progress t in [0,1].
     * @param t Progress along path (0.0 = start, 1.0 = end)
     * @return Tangent angle in radians
     */
    [[nodiscard]] double sample_tangent_angle(double t) const {
        if (empty()) return 0.0;

        t = std::clamp(t, 0.0, 1.0);

        const auto& vertices = subpaths.empty() ? points : subpaths[0].vertices;
        if (vertices.size() < 2) return 0.0;

        double total_segments = static_cast<double>(vertices.size() - 1);
        double segment_float = t * total_segments;
        int segment = static_cast<int>(segment_float);

        if (segment >= static_cast<int>(vertices.size()) - 1) {
            segment = static_cast<int>(vertices.size()) - 2;
        }

        const auto& v0 = vertices[segment];
        const auto& v1 = vertices[segment + 1];

        double dx = v1.point.x - v0.point.x;
        double dy = v1.point.y - v0.point.y;

        return std::atan2(dy, dx);
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
