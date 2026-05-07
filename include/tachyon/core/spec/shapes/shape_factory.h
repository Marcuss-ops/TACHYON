#pragma once

#include "tachyon/core/spec/schema/objects/path_spec.h"
#include <vector>

namespace tachyon::spec {

/**
 * @brief Factory for creating path geometry for common shapes.
 */
class ShapeFactory {
public:
    // Primitives
    static PathGeometry create_rectangle(float x, float y, float width, float height);
    static PathGeometry create_rounded_rectangle(float x, float y, float width, float height, float radius);
    static PathGeometry create_circle(float cx, float cy, float radius);
    static PathGeometry create_ellipse(float cx, float cy, float rx, float ry);
    static PathGeometry create_line(float x0, float y0, float x1, float y1);
    static PathGeometry create_arrow(float x0, float y0, float x1, float y1, float head_size = 10.0f);
    static PathGeometry create_polygon(float cx, float cy, int sides, float radius);
    static PathGeometry create_star(float cx, float cy, int points, float inner_radius, float outer_radius);
    
    // UI / Overlay Specific
    static PathGeometry create_speech_bubble(float x, float y, float w, float h, float radius, float tail_x, float tail_y);
    static PathGeometry create_callout(float x, float y, float w, float h, float target_x, float target_y);
    static PathGeometry create_badge(float cx, float cy, float radius, int points = 16);
    
    // Path modification
    static PathGeometry dash_path(const PathGeometry& path, const std::vector<float>& dash_array, float dash_offset = 0.0f);
};

} // namespace tachyon::spec
