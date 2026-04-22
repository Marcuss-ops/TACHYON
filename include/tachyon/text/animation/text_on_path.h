#pragma once

#include "tachyon/text/core/layout/resolved_text_layout.h"
#include "tachyon/core/shapes/shape_path.h"
#include <cmath>
#include <vector>

namespace tachyon::text {

class TextOnPathModifier {
public:
    /**
     * @brief Maps the glyphs in a ResolvedTextLayout onto a ShapePath.
     * Modifies the glyph positions and rotations in-place based on true arc-length parameterization.
     */
    static void apply(
        ResolvedTextLayout& layout,
        const shapes::ShapePath& path,
        double path_offset,
        bool align_perpendicular = true) {
        
        if (layout.glyphs.empty() || path.empty() || path.subpaths.empty()) return;

        layout.is_on_path = true;

        // 1. Compute total arc-length LUT across ALL subpaths
        const int samples = 50;
        std::vector<double> arc_lengths;
        std::vector<shapes::Point2D> points_lut;
        double total_length = 0.0;

        for (const auto& subpath : path.subpaths) {
            if (subpath.vertices.empty()) continue;
            
            size_t start_idx = points_lut.size();
            points_lut.push_back(subpath.vertices[0].point);
            if (arc_lengths.empty()) arc_lengths.push_back(0.0);
            else arc_lengths.push_back(total_length);

            for (std::size_t i = 1; i < subpath.vertices.size(); ++i) {
                const auto& v0 = subpath.vertices[i - 1];
                const auto& v1 = subpath.vertices[i];
                
                shapes::Point2D p0 = v0.point;
                shapes::Point2D p1 = {v0.point.x + v0.out_tangent.x, v0.point.y + v0.out_tangent.y};
                shapes::Point2D p2 = {v1.point.x + v1.in_tangent.x, v1.point.y + v1.in_tangent.y};
                shapes::Point2D p3 = v1.point;
                
                for (int s = 1; s <= samples; ++s) {
                    double t = static_cast<double>(s) / samples;
                    double it = 1.0 - t;
                    double it2 = it * it;
                    double it3 = it2 * it;
                    double t2 = t * t;
                    double t3 = t2 * t;
                    
                    shapes::Point2D pt = {
                        it3 * p0.x + 3 * it2 * t * p1.x + 3 * it * t2 * p2.x + t3 * p3.x,
                        it3 * p0.y + 3 * it2 * t * p1.y + 3 * it * t2 * p2.y + t3 * p3.y
                    };
                    
                    double dist = std::sqrt(std::pow(pt.x - points_lut.back().x, 2) + std::pow(pt.y - points_lut.back().y, 2));
                    total_length += dist;
                    
                    arc_lengths.push_back(total_length);
                    points_lut.push_back(pt);
                }
            }
            // Handle closed subpath connection
            if (subpath.closed) {
                const auto& v0 = subpath.vertices.back();
                const auto& v1 = subpath.vertices[0];
                shapes::Point2D p0 = v0.point;
                shapes::Point2D p1 = {v0.point.x + v0.out_tangent.x, v0.point.y + v0.out_tangent.y};
                shapes::Point2D p2 = {v1.point.x + v1.in_tangent.x, v1.point.y + v1.in_tangent.y};
                shapes::Point2D p3 = v1.point;
                for (int s = 1; s <= samples; ++s) {
                    double t = static_cast<double>(s) / samples;
                    double it = 1.0 - t;
                    double t2 = t * t;
                    double t3 = t2 * t;
                    shapes::Point2D pt = {
                        (1-t)*(1-t)*(1-t) * p0.x + 3*(1-t)*(1-t)*t * p1.x + 3*(1-t)*t*t * p2.x + t3 * p3.x,
                        (1-t)*(1-t)*(1-t) * p0.y + 3*(1-t)*(1-t)*t * p1.y + 3*(1-t)*t*t * p2.y + t3 * p3.y
                    };
                    double dist = std::sqrt(std::pow(pt.x - points_lut.back().x, 2) + std::pow(pt.y - points_lut.back().y, 2));
                    total_length += dist;
                    arc_lengths.push_back(total_length);
                    points_lut.push_back(pt);
                }
            }
        }

        if (total_length <= 0.0) return;

        // 2. Map glyphs onto the path
        double layout_start_x = layout.total_bounds.x;
        bool is_closed_any = std::any_of(path.subpaths.begin(), path.subpaths.end(), [](const auto& s){ return s.closed; });
        
        for (auto& glyph : layout.glyphs) {
            double glyph_x = glyph.position.x - layout_start_x + (glyph.bounds.width * 0.5);
            double target_dist = glyph_x + path_offset;
            
            if (is_closed_any) {
                target_dist = std::fmod(target_dist, total_length);
                if (target_dist < 0) target_dist += total_length;
            } else {
                target_dist = std::clamp(target_dist, 0.0, total_length);
            }
            
            auto it = std::lower_bound(arc_lengths.begin(), arc_lengths.end(), target_dist);
            std::size_t idx = 0;
            if (it == arc_lengths.begin()) idx = 0;
            else if (it == arc_lengths.end()) idx = arc_lengths.size() - 2;
            else idx = std::distance(arc_lengths.begin(), it) - 1;

            double d0 = arc_lengths[idx];
            double d1 = arc_lengths[idx + 1];
            double f = (d1 - d0) > 1e-6 ? (target_dist - d0) / (d1 - d0) : 0.0;
            
            shapes::Point2D pt0 = points_lut[idx];
            shapes::Point2D pt1 = points_lut[idx + 1];
            
            shapes::Point2D final_pos = {
                pt0.x + (pt1.x - pt0.x) * f,
                pt0.y + (pt1.y - pt0.y) * f
            };
            
            if (align_perpendicular) {
                double dx = pt1.x - pt0.x;
                double dy = pt1.y - pt0.y;
                double angle_rad = std::atan2(dy, dx);
                
                glyph.rotation += static_cast<float>(angle_rad * 180.0 / 3.14159265358979323846);
                
                double baseline_offset = glyph.position.y - layout.total_bounds.y;
                final_pos.x -= std::sin(angle_rad) * baseline_offset;
                final_pos.y += std::cos(angle_rad) * baseline_offset;
            }
            
            glyph.position.x = static_cast<float>(final_pos.x - (glyph.bounds.width * 0.5));
            glyph.position.y = static_cast<float>(final_pos.y);
        }
    }
};

} // namespace tachyon::text
