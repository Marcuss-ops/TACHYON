#include "tachyon/core/spec/shapes/shape_factory.h"
#include "tachyon/core/math/math_utils.h"
#include <cmath>
#include <algorithm>

namespace tachyon::spec {

namespace {
constexpr float KAPPA = 0.5522847498f; // (4/3)*(sqrt(2)-1) for circle approximation
}

PathGeometry ShapeFactory::create_rectangle(float x, float y, float width, float height) {
    PathGeometry path;
    path.commands.push_back({PathVerb::MoveTo, {x, y}});
    path.commands.push_back({PathVerb::LineTo, {x + width, y}});
    path.commands.push_back({PathVerb::LineTo, {x + width, y + height}});
    path.commands.push_back({PathVerb::LineTo, {x, y + height}});
    path.commands.push_back({PathVerb::Close});
    return path;
}

PathGeometry ShapeFactory::create_rounded_rectangle(float x, float y, float width, float height, float radius) {
    radius = std::min({radius, width * 0.5f, height * 0.5f});
    if (radius <= 0.0f) return create_rectangle(x, y, width, height);

    PathGeometry path;
    const float k = radius * KAPPA;
    
    path.commands.push_back({PathVerb::MoveTo, {x + radius, y}});
    
    // Top edge
    path.commands.push_back({PathVerb::LineTo, {x + width - radius, y}});
    // Top-right corner
    path.commands.push_back({PathVerb::CubicTo, {x + width - radius + k, y}, {x + width, y + radius - k}, {x + width, y + radius}});
    
    // Right edge
    path.commands.push_back({PathVerb::LineTo, {x + width, y + height - radius}});
    // Bottom-right corner
    path.commands.push_back({PathVerb::CubicTo, {x + width, y + height - radius + k}, {x + width - radius + k, y + height}, {x + width - radius, y + height}});
    
    // Bottom edge
    path.commands.push_back({PathVerb::LineTo, {x + radius, y + height}});
    // Bottom-left corner
    path.commands.push_back({PathVerb::CubicTo, {x + radius - k, y + height}, {x, y + height - radius + k}, {x, y + height - radius}});
    
    // Left edge
    path.commands.push_back({PathVerb::LineTo, {x, y + radius}});
    // Top-left corner
    path.commands.push_back({PathVerb::CubicTo, {x, y + radius - k}, {x + radius - k, y}, {x + radius, y}});
    
    path.commands.push_back({PathVerb::Close});
    return path;
}

PathGeometry ShapeFactory::create_circle(float cx, float cy, float radius) {
    return create_ellipse(cx, cy, radius, radius);
}

PathGeometry ShapeFactory::create_ellipse(float cx, float cy, float rx, float ry) {
    PathGeometry path;
    const float kx = rx * KAPPA;
    const float ky = ry * KAPPA;
    
    path.commands.push_back({PathVerb::MoveTo, {cx + rx, cy}});
    path.commands.push_back({PathVerb::CubicTo, {cx + rx, cy + ky}, {cx + kx, cy + ry}, {cx, cy + ry}});
    path.commands.push_back({PathVerb::CubicTo, {cx - kx, cy + ry}, {cx - rx, cy + ky}, {cx - rx, cy}});
    path.commands.push_back({PathVerb::CubicTo, {cx - rx, cy - ky}, {cx - kx, cy - ry}, {cx, cy - ry}});
    path.commands.push_back({PathVerb::CubicTo, {cx + kx, cy - ry}, {cx + rx, cy - ky}, {cx + rx, cy}});
    path.commands.push_back({PathVerb::Close});
    return path;
}

PathGeometry ShapeFactory::create_line(float x0, float y0, float x1, float y1) {
    PathGeometry path;
    path.commands.push_back({PathVerb::MoveTo, {x0, y0}});
    path.commands.push_back({PathVerb::LineTo, {x1, y1}});
    return path;
}

PathGeometry ShapeFactory::create_arrow(float x0, float y0, float x1, float y1, float head_size) {
    PathGeometry path;
    math::Vector2 start{x0, y0};
    math::Vector2 end{x1, y1};
    math::Vector2 diff = end - start;
    float len = diff.length();
    if (len < 1e-6f) return path;

    math::Vector2 dir = diff / len;
    math::Vector2 perp{-dir.y, dir.x};
    
    path.commands.push_back({PathVerb::MoveTo, start});
    path.commands.push_back({PathVerb::LineTo, end});
    
    math::Vector2 head_base = end - dir * head_size;
    math::Vector2 p1 = head_base + perp * (head_size * 0.5f);
    math::Vector2 p2 = head_base - perp * (head_size * 0.5f);
    
    path.commands.push_back({PathVerb::MoveTo, p1});
    path.commands.push_back({PathVerb::LineTo, end});
    path.commands.push_back({PathVerb::LineTo, p2});
    
    return path;
}

PathGeometry ShapeFactory::create_polygon(float cx, float cy, int sides, float radius) {
    PathGeometry path;
    if (sides < 3) return path;
    
    const float pi_f = static_cast<float>(math::kPi);
    for (int i = 0; i < sides; ++i) {
        float angle = static_cast<float>(i) * 2.0f * pi_f / static_cast<float>(sides) - pi_f * 0.5f;
        math::Vector2 p{cx + radius * std::cos(angle), cy + radius * std::sin(angle)};
        if (i == 0) {
            path.commands.push_back({PathVerb::MoveTo, p});
        } else {
            path.commands.push_back({PathVerb::LineTo, p});
        }
    }
    path.commands.push_back({PathVerb::Close});
    return path;
}

PathGeometry ShapeFactory::create_star(float cx, float cy, int points, float inner_radius, float outer_radius) {
    PathGeometry path;
    if (points < 2) return path;
    
    const float pi_f = static_cast<float>(math::kPi);
    int num_vertices = points * 2;
    for (int i = 0; i < num_vertices; ++i) {
        float r = (i % 2 == 0) ? outer_radius : inner_radius;
        float angle = static_cast<float>(i) * pi_f / static_cast<float>(points) - pi_f * 0.5f;
        math::Vector2 p{cx + r * std::cos(angle), cy + r * std::sin(angle)};
        if (i == 0) {
            path.commands.push_back({PathVerb::MoveTo, p});
        } else {
            path.commands.push_back({PathVerb::LineTo, p});
        }
    }
    path.commands.push_back({PathVerb::Close});
    return path;
}

PathGeometry ShapeFactory::create_speech_bubble(float x, float y, float w, float h, float radius, float tail_x, float tail_y) {
    PathGeometry path = create_rounded_rectangle(x, y, w, h, radius);
    
    if (!path.commands.empty() && path.commands.back().verb == PathVerb::Close) {
        path.commands.pop_back();
    }
    
    math::Vector2 center{x + w * 0.5f, y + h * 0.5f};
    math::Vector2 tail_target{tail_x, tail_y};
    math::Vector2 to_tail_full = tail_target - center;
    float to_tail_len = to_tail_full.length();
    if (to_tail_len < 1e-6f) return path;

    math::Vector2 to_tail = to_tail_full / to_tail_len;
    math::Vector2 perp{-to_tail.y, to_tail.x};
    
    float tail_width = 20.0f;
    math::Vector2 b1 = center + to_tail * (std::min(w, h) * 0.4f) + perp * (tail_width * 0.5f);
    math::Vector2 b2 = center + to_tail * (std::min(w, h) * 0.4f) - perp * (tail_width * 0.5f);
    
    path.commands.push_back({PathVerb::MoveTo, b1});
    path.commands.push_back({PathVerb::LineTo, tail_target});
    path.commands.push_back({PathVerb::LineTo, b2});
    path.commands.push_back({PathVerb::Close});
    
    return path;
}

PathGeometry ShapeFactory::create_callout(float x, float y, float w, float h, float target_x, float target_y) {
     return create_speech_bubble(x, y, w, h, 5.0f, target_x, target_y);
}

PathGeometry ShapeFactory::create_badge(float cx, float cy, float radius, int points) {
    return create_star(cx, cy, points, radius * 0.85f, radius);
}

PathGeometry ShapeFactory::dash_path(const PathGeometry& path, const std::vector<float>& dash_array, float dash_offset) {
    if (dash_array.empty()) return path;
    
    PathGeometry result;
    if (path.commands.empty()) return result;

    float pattern_total = 0.0f;
    for (float d : dash_array) pattern_total += d;
    if (pattern_total <= 0.0f) return path;

    math::Vector2 current_p{0, 0};
    math::Vector2 move_to_p{0, 0};
    float distance_acc = dash_offset;

    auto wrap_dist = [&](float d) {
        d = std::fmod(d, pattern_total);
        if (d < 0.0f) d += pattern_total;
        return d;
    };

    distance_acc = wrap_dist(distance_acc);

    for (const auto& cmd : path.commands) {
        if (cmd.verb == PathVerb::MoveTo) {
            current_p = cmd.p0;
            move_to_p = cmd.p0;
        } else if (cmd.verb == PathVerb::LineTo || (cmd.verb == PathVerb::Close && (current_p - move_to_p).length() > 1e-6f)) {
            math::Vector2 target = (cmd.verb == PathVerb::LineTo) ? cmd.p0 : move_to_p;
            math::Vector2 diff = target - current_p;
            float seg_len = diff.length();
            if (seg_len < 1e-6f) {
                current_p = target;
                continue;
            }

            math::Vector2 dir = diff / seg_len;
            
            float remaining = seg_len;
            while (remaining > 0.0f) {
                float acc = 0.0f;
                int dash_idx = 0;
                bool is_on = true;
                for (size_t i = 0; i < dash_array.size(); ++i) {
                    if (distance_acc < acc + dash_array[i]) {
                        dash_idx = (int)i;
                        is_on = (i % 2 == 0);
                        break;
                    }
                    acc += dash_array[i];
                }
                
                float dash_rem = (acc + dash_array[dash_idx]) - distance_acc;
                float step = std::min(remaining, dash_rem);
                
                if (is_on) {
                    result.commands.push_back({PathVerb::MoveTo, current_p});
                    result.commands.push_back({PathVerb::LineTo, current_p + dir * step});
                }
                
                current_p += dir * step;
                remaining -= step;
                distance_acc = wrap_dist(distance_acc + step);
            }
            current_p = target;
        }
    }
    
    return result;
}

} // namespace tachyon::spec
