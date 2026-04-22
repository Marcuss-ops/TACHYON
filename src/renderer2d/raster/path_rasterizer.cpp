#include "tachyon/renderer2d/raster/path_rasterizer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace tachyon::renderer2d {
namespace {

struct Contour {
    std::vector<math::Vector2> points;
};

constexpr int kCoverageSamples = 4;

Color apply_coverage(Color color, float opacity, float coverage) {
    const float alpha = color.a * std::clamp(opacity, 0.0f, 1.0f) * std::clamp(coverage, 0.0f, 1.0f);
    color.a = alpha;
    return color;
}

Color sample_gradient(const GradientSpec& grad, float x, float y) {
    if (grad.stops.empty()) return Color::white();
    if (grad.stops.size() == 1) return Color{grad.stops[0].color.r / 255.0f, grad.stops[0].color.g / 255.0f, grad.stops[0].color.b / 255.0f, grad.stops[0].color.a / 255.0f};

    float t = 0.0f;
    const math::Vector2 p{x, y};
    if (grad.type == GradientType::Linear) {
        const math::Vector2 ab = grad.end - grad.start;
        const float len2 = ab.length_squared();
        if (len2 > 1e-6f) {
            t = math::Vector2::dot(p - grad.start, ab) / len2;
        }
    } else {
        const float d = (p - grad.start).length();
        if (grad.radial_radius > 1e-6f) {
            t = d / grad.radial_radius;
        }
    }
    t = std::clamp(t, 0.0f, 1.0f);

    // Find stops
    const auto it = std::lower_bound(grad.stops.begin(), grad.stops.end(), t, [](const GradientStop& s, float val) {
        return s.offset < val;
    });

    if (it == grad.stops.begin()) return Color{it->color.r / 255.0f, it->color.g / 255.0f, it->color.b / 255.0f, it->color.a / 255.0f};
    if (it == grad.stops.end()) {
        const auto& last = grad.stops.back();
        return Color{last.color.r / 255.0f, last.color.g / 255.0f, last.color.b / 255.0f, last.color.a / 255.0f};
    }

    const auto& s1 = *(it - 1);
    const auto& s2 = *it;
    const float range = s2.offset - s1.offset;
    const float alpha = (range > 1e-6f) ? (t - s1.offset) / range : 0.0f;
    
    return Color{
        (s1.color.r * (1.0f - alpha) + s2.color.r * alpha) / 255.0f,
        (s1.color.g * (1.0f - alpha) + s2.color.g * alpha) / 255.0f,
        (s1.color.b * (1.0f - alpha) + s2.color.b * alpha) / 255.0f,
        (s1.color.a * (1.0f - alpha) + s2.color.a * alpha) / 255.0f
    };
}

float distance_to_segment_squared(const math::Vector2& point, const math::Vector2& a, const math::Vector2& b) {
    const math::Vector2 ab = b - a;
    const float length_squared = ab.length_squared();
    if (length_squared <= std::numeric_limits<float>::epsilon()) {
        return (point - a).length_squared();
    }

    const float t = std::clamp(math::Vector2::dot(point - a, ab) / length_squared, 0.0f, 1.0f);
    const math::Vector2 projection = a + ab * t;
    return (point - projection).length_squared();
}

void flatten_cubic(
    const math::Vector2& p0,
    const math::Vector2& p1,
    const math::Vector2& p2,
    const math::Vector2& p3,
    std::vector<math::Vector2>& out,
    float tolerance,
    std::uint32_t depth = 0U) {

    const math::Vector2 chord = p3 - p0;
    const float chord_length = chord.length();
    const float distance_1 = chord_length > 0.0f ? std::abs((p1.x - p0.x) * chord.y - (p1.y - p0.y) * chord.x) / chord_length : (p1 - p0).length();
    const float distance_2 = chord_length > 0.0f ? std::abs((p2.x - p0.x) * chord.y - (p2.y - p0.y) * chord.x) / chord_length : (p2 - p0).length();

    if ((std::max(distance_1, distance_2) <= tolerance) || depth >= 12U) {
        out.push_back(p3);
        return;
    }

    const math::Vector2 p01 = (p0 + p1) * 0.5f;
    const math::Vector2 p12 = (p1 + p2) * 0.5f;
    const math::Vector2 p23 = (p2 + p3) * 0.5f;
    const math::Vector2 p012 = (p01 + p12) * 0.5f;
    const math::Vector2 p123 = (p12 + p23) * 0.5f;
    const math::Vector2 p0123 = (p012 + p123) * 0.5f;

    flatten_cubic(p0, p01, p012, p0123, out, tolerance, depth + 1U);
    flatten_cubic(p0123, p123, p23, p3, out, tolerance, depth + 1U);
}

std::vector<Contour> build_contours(const PathGeometry& path) {
    constexpr float kCloseEpsilon = 0.01f;
    std::vector<Contour> contours;
    Contour current;
    math::Vector2 current_point{};
    bool has_current_point = false;
    bool contour_open = false;

    auto close_contour = [&]() {
        if (contour_open && current.points.size() >= 2U) {
            const math::Vector2& first = current.points.front();
            const math::Vector2& last = current.points.back();
            if (std::abs(first.x - last.x) > kCloseEpsilon || std::abs(first.y - last.y) > kCloseEpsilon) {
                current.points.push_back(current.points.front());
            }
        }
        if (!current.points.empty()) {
            contours.push_back(std::move(current));
        }
        current = {};
        contour_open = false;
        has_current_point = false;
    };

    for (const PathCommand& command : path.commands) {
        switch (command.verb) {
            case PathVerb::MoveTo:
                close_contour();
                current.points.push_back(command.p0);
                current_point = command.p0;
                has_current_point = true;
                contour_open = true;
                break;
            case PathVerb::LineTo:
                if (!has_current_point) {
                    current.points.push_back(command.p0);
                    current_point = command.p0;
                    has_current_point = true;
                    contour_open = true;
                } else {
                    current.points.push_back(command.p0);
                    current_point = command.p0;
                }
                break;
            case PathVerb::CubicTo:
                if (!has_current_point) {
                    current.points.push_back(command.p0);
                    current_point = command.p0;
                    has_current_point = true;
                    contour_open = true;
                }
                flatten_cubic(current_point, command.p0, command.p1, command.p2, current.points, 0.35f);
                current_point = command.p2;
                break;
            case PathVerb::Close:
                close_contour();
                break;
        }
    }

    if (!current.points.empty()) {
        close_contour();
    }

    return contours;
}

void rasterize_fill_polygon(SurfaceRGBA& surface, const std::vector<Contour>& contours, const FillPathStyle& style) {
    if (contours.empty()) return;

    // 1. Calculate Bounding Box
    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float max_y = std::numeric_limits<float>::lowest();

    for (const auto& contour : contours) {
        for (const auto& p : contour.points) {
            min_x = std::min(min_x, p.x);
            min_y = std::min(min_y, p.y);
            max_x = std::max(max_x, p.x);
            max_y = std::max(max_y, p.y);
        }
    }

    const int start_y = std::max(0, static_cast<int>(std::floor(min_y)));
    const int end_y = std::min(static_cast<int>(surface.height()), static_cast<int>(std::ceil(max_y)));
    const int width = static_cast<int>(surface.width());

    // 2. Per-scanline Analytical Accumulation
    std::vector<float> coverage_buffer(width + 1, 0.0f);

    for (int y = start_y; y < end_y; ++y) {
        std::fill(coverage_buffer.begin(), coverage_buffer.end(), 0.0f);

        for (const auto& contour : contours) {
            for (size_t i = 0; i + 1 < contour.points.size(); ++i) {
                const auto& p1 = contour.points[i];
                const auto& p2 = contour.points[i + 1];

                float y1 = p1.y, y2 = p2.y;
                if (std::max(y1, y2) <= (float)y || std::min(y1, y2) >= (float)y + 1) continue;

                float row_y1 = std::max(y1, (float)y);
                (void)row_y1; // unused for now, but keeping sy1/sy2 logic
                
                // Re-calculating row boundaries correctly
                float sy1 = std::max(std::min(y1, y2), (float)y);
                float sy2 = std::min(std::max(y1, y2), (float)y + 1);
                if (sy1 >= sy2) continue;

                auto get_x = [&](float py) {
                    if (std::abs(y2 - y1) < 1e-6f) return p1.x;
                    return p1.x + (p2.x - p1.x) * (py - y1) / (y2 - y1);
                };

                float x1 = get_x(sy1);
                float x2 = get_x(sy2);
                float height = sy2 - sy1;
                if (y1 > y2) height = -height;

                const float min_x_seg = std::min(x1, x2);
                const float max_x_seg = std::max(x1, x2);
                const int ix1 = static_cast<int>(std::floor(min_x_seg));
                const int ix2 = static_cast<int>(std::floor(max_x_seg));

                if (ix1 == ix2) {
                    // Single pixel
                    const float area = height * (1.0f - (x1 + x2) * 0.5f + static_cast<float>(ix1));
                    if (ix1 >= 0 && ix1 < width) coverage_buffer[ix1] += area;
                } else {
                    // Multiple pixels
                    const float slope = (x2 - x1) / (sy2 - sy1);
                    const float dir = (y2 > y1) ? 1.0f : -1.0f;
                    
                    // First pixel
                    float next_ix_x = static_cast<float>(ix1 + 1);
                    float next_sy = sy1 + (next_ix_x - x1) / slope;
                    float h = next_sy - sy1;
                    coverage_buffer[std::clamp(ix1, 0, width)] += h * dir * (1.0f - (x1 + next_ix_x) * 0.5f + static_cast<float>(ix1));
                    
                    // Intermediate pixels
                    for (int ix = ix1 + 1; ix < ix2; ++ix) {
                        float cur_sy = next_sy;
                        next_ix_x = static_cast<float>(ix + 1);
                        next_sy = sy1 + (next_ix_x - x1) / slope;
                        h = next_sy - cur_sy;
                        coverage_buffer[std::clamp(ix, 0, width)] += h * dir * 0.5f;
                    }
                    
                    // Last pixel
                    float final_h = sy2 - next_sy;
                    coverage_buffer[std::clamp(ix2, 0, width)] += final_h * dir * (1.0f - (next_ix_x + x2) * 0.5f + static_cast<float>(ix2));
                }

                // Add height to the rest of the scanline for winding accumulation
                const int next_x = std::max(0, ix2 + 1);
                if (next_x < width) coverage_buffer[next_x] += height;
            }
        }

        // 3. Accumulate and Blend
        float winding = 0.0f;
        for (int x = 0; x < width; ++x) {
            winding += coverage_buffer[x];
            float coverage = 0.0f;
            if (style.winding == WindingRule::NonZero) {
                coverage = std::clamp(std::abs(winding), 0.0f, 1.0f);
            } else {
                // Even-Odd: winding is periodic with period 2
                float w = std::abs(winding);
                float rem = std::fmod(w, 2.0f);
                coverage = (rem > 1.0f) ? 2.0f - rem : rem;
            }

            if (coverage > 0.001f) {
                Color c = style.fill_color;
                if (style.gradient.has_value()) {
                    c = sample_gradient(*style.gradient, static_cast<float>(x), static_cast<float>(y));
                }
                surface.blend_pixel(
                    static_cast<std::uint32_t>(x),
                    static_cast<std::uint32_t>(y),
                    apply_coverage(c, style.opacity, coverage));
            }
        }
    }
}

float stroke_coverage(const std::vector<Contour>& contours, int x, int y, float radius, const StrokePathStyle& style) {
    (void)style;
    float min_dist_sq = std::numeric_limits<float>::max();
    math::Vector2 p{static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f};
    for (const auto& contour : contours) {
        for (size_t i = 0; i + 1 < contour.points.size(); ++i) {
            min_dist_sq = std::min(min_dist_sq, distance_to_segment_squared(p, contour.points[i], contour.points[i+1]));
        }
    }
    float dist = std::sqrt(min_dist_sq);
    return std::clamp(radius - dist + 0.5f, 0.0f, 1.0f);
}

void rasterize_stroke_polygon(SurfaceRGBA& surface, const std::vector<Contour>& contours, const StrokePathStyle& style) {
    if (contours.empty() || style.stroke_width <= 0.0f) {
        return;
    }

    const float radius = std::max(0.5f, style.stroke_width * 0.5f);

    for (const Contour& contour : contours) {
        if (contour.points.size() < 2U) {
            continue;
        }

        float min_x = std::numeric_limits<float>::max();
        float min_y = std::numeric_limits<float>::max();
        float max_x = std::numeric_limits<float>::lowest();
        float max_y = std::numeric_limits<float>::lowest();

        for (const math::Vector2& point : contour.points) {
            min_x = std::min(min_x, point.x);
            min_y = std::min(min_y, point.y);
            max_x = std::max(max_x, point.x);
            max_y = std::max(max_y, point.y);
        }

        const int start_x = std::max(0, static_cast<int>(std::floor(min_x - radius)));
        const int start_y = std::max(0, static_cast<int>(std::floor(min_y - radius)));
        const int end_x = std::min(static_cast<int>(surface.width()), static_cast<int>(std::ceil(max_x + radius)));
        const int end_y = std::min(static_cast<int>(surface.height()), static_cast<int>(std::ceil(max_y + radius)));

        for (int y = start_y; y < end_y; ++y) {
            for (int x = start_x; x < end_x; ++x) {
                const float coverage = stroke_coverage(contours, x, y, radius, style);
                if (coverage > 0.0f) {
                    Color c = style.stroke_color;
                    if (style.gradient.has_value()) {
                        c = sample_gradient(*style.gradient, static_cast<float>(x), static_cast<float>(y));
                    }
                    surface.blend_pixel(
                        static_cast<std::uint32_t>(x),
                        static_cast<std::uint32_t>(y),
                        apply_coverage(c, style.opacity, coverage));
                }
            }
        }
    }
}

} // namespace

namespace {

float segment_length(const math::Vector2& p0, const math::Vector2& p1, const math::Vector2& p2, const math::Vector2& p3, PathVerb verb) {
    if (verb == PathVerb::LineTo) {
        return (p1 - p0).length();
    } else if (verb == PathVerb::CubicTo) {
        std::vector<math::Vector2> flattened;
        flattened.push_back(p0);
        flatten_cubic(p0, p1, p2, p3, flattened, 0.5f);
        float len = 0.0f;
        for (std::size_t i = 0; i + 1 < flattened.size(); ++i) {
            len += (flattened[i+1] - flattened[i]).length();
        }
        return len;
    }
    return 0.0f;
}

void split_cubic(const math::Vector2& p0, const math::Vector2& p1, const math::Vector2& p2, const math::Vector2& p3, float t, math::Vector2* left, math::Vector2* right) {
    const math::Vector2 p01 = p0 * (1.0f - t) + p1 * t;
    const math::Vector2 p12 = p1 * (1.0f - t) + p2 * t;
    const math::Vector2 p23 = p2 * (1.0f - t) + p3 * t;
    const math::Vector2 p012 = p01 * (1.0f - t) + p12 * t;
    const math::Vector2 p123 = p12 * (1.0f - t) + p23 * t;
    const math::Vector2 p0123 = p012 * (1.0f - t) + p123 * t;
    
    if (left) {
        left[0] = p0;
        left[1] = p01;
        left[2] = p012;
        left[3] = p0123;
    }
    if (right) {
        right[0] = p0123;
        right[1] = p123;
        right[2] = p23;
        right[3] = p3;
    }
}

void extract_sub_cubic(const math::Vector2& p0, const math::Vector2& p1, const math::Vector2& p2, const math::Vector2& p3, float t1, float t2, math::Vector2* out) {
    if (t1 <= 0.0f && t2 >= 1.0f) {
        out[0] = p0; out[1] = p1; out[2] = p2; out[3] = p3;
        return;
    }
    if (t1 >= 1.0f || t2 <= 0.0f || t1 >= t2) {
        out[0] = out[1] = out[2] = out[3] = (t1 >= 1.0f ? p3 : p0);
        return;
    }

    math::Vector2 right[4];
    split_cubic(p0, p1, p2, p3, t1, nullptr, right);
    
    const float nt2 = (t2 - t1) / (1.0f - t1);
    split_cubic(right[0], right[1], right[2], right[3], nt2, out, nullptr);
}

} // namespace

void PathRasterizer::fill(SurfaceRGBA& surface, const PathGeometry& path, const FillPathStyle& style) {
    const std::vector<Contour> contours = build_contours(path);
    rasterize_fill_polygon(surface, contours, style);
}

void PathRasterizer::stroke(SurfaceRGBA& surface, const PathGeometry& path, const StrokePathStyle& style) {
    const std::vector<Contour> contours = build_contours(path);
    rasterize_stroke_polygon(surface, contours, style);
}

PathGeometry PathRasterizer::trim(const PathGeometry& path, float start, float end, float offset) {
    if (path.commands.empty()) return path;
    
    auto wrap = [](float v) {
        v = std::fmod(v, 1.0f);
        if (v < 0.0f) v += 1.0f;
        return v;
    };
    
    const float t_start = wrap(start + offset);
    const float t_end = wrap(end + offset);
    
    struct Segment {
        PathVerb verb;
        math::Vector2 p[4]; 
        float length;
        float cum_length;
    };
    
    std::vector<Segment> segments;
    float total_length = 0.0f;
    math::Vector2 current_point{0, 0};
    math::Vector2 move_to_point{0, 0};
    
    for (const auto& cmd : path.commands) {
        if (cmd.verb == PathVerb::MoveTo) {
            current_point = cmd.p0;
            move_to_point = cmd.p0;
        } else if (cmd.verb == PathVerb::LineTo) {
            float len = (cmd.p0 - current_point).length();
            segments.push_back({PathVerb::LineTo, {current_point, cmd.p0, {}, {}}, len, 0.0f});
            total_length += len;
            current_point = cmd.p0;
        } else if (cmd.verb == PathVerb::CubicTo) {
            float len = segment_length(current_point, cmd.p0, cmd.p1, cmd.p2, PathVerb::CubicTo);
            segments.push_back({PathVerb::CubicTo, {current_point, cmd.p0, cmd.p1, cmd.p2}, len, 0.0f});
            total_length += len;
            current_point = cmd.p2;
        } else if (cmd.verb == PathVerb::Close) {
             float len = (move_to_point - current_point).length();
             if (len > 1e-6f) {
                 segments.push_back({PathVerb::LineTo, {current_point, move_to_point, {}, {}}, len, 0.0f});
                 total_length += len;
             }
             current_point = move_to_point;
        }
    }
    
    if (total_length < 1e-6f) return path;
    
    float acc = 0.0f;
    for (auto& seg : segments) {
        seg.cum_length = acc;
        acc += seg.length;
    }
    
    auto collect_range = [&](float s, float e, PathGeometry& out) {
        const float start_len = s * total_length;
        const float end_len = e * total_length;
        
        bool first_move = true;
        
        for (const auto& seg : segments) {
            const float s_start = seg.cum_length;
            const float s_end = seg.cum_length + seg.length;
            
            if (s_end < start_len || s_start > end_len) continue;
            
            const float local_s = std::clamp((start_len - s_start) / seg.length, 0.0f, 1.0f);
            const float local_e = std::clamp((end_len - s_start) / seg.length, 0.0f, 1.0f);
            
            if (local_s >= local_e - 1e-6f) continue;
            
            if (seg.verb == PathVerb::LineTo) {
                const math::Vector2 p_start = seg.p[0] * (1.0f - local_s) + seg.p[1] * local_s;
                const math::Vector2 p_end = seg.p[0] * (1.0f - local_e) + seg.p[1] * local_e;
                if (first_move) {
                    out.commands.push_back({PathVerb::MoveTo, p_start});
                    first_move = false;
                }
                out.commands.push_back({PathVerb::LineTo, p_end});
            } else if (seg.verb == PathVerb::CubicTo) {
                math::Vector2 sub[4];
                extract_sub_cubic(seg.p[0], seg.p[1], seg.p[2], seg.p[3], local_s, local_e, sub);
                if (first_move) {
                    out.commands.push_back({PathVerb::MoveTo, sub[0]});
                    first_move = false;
                }
                out.commands.push_back({PathVerb::CubicTo, sub[1], sub[2], sub[3]});
            }
        }
    };
    
    PathGeometry result;
    if (t_start <= t_end) {
        collect_range(t_start, t_end, result);
    } else {
        collect_range(t_start, 1.0f, result);
        collect_range(0.0f, t_end, result);
    }
    
    return result;
}

} // namespace tachyon::renderer2d
