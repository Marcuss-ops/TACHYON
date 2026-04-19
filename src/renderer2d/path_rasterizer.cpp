#include "tachyon/renderer2d/path_rasterizer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace tachyon::renderer2d {
namespace {

struct Contour {
    std::vector<math::Vector2> points;
};

Color apply_opacity(Color color, float opacity) {
    const float clamped = std::clamp(opacity, 0.0f, 1.0f);
    const float alpha = static_cast<float>(color.a) * clamped;
    const float premultiplied = alpha / 255.0f;
    return Color{
        static_cast<std::uint8_t>(std::lround(static_cast<float>(color.r) * premultiplied)),
        static_cast<std::uint8_t>(std::lround(static_cast<float>(color.g) * premultiplied)),
        static_cast<std::uint8_t>(std::lround(static_cast<float>(color.b) * premultiplied)),
        static_cast<std::uint8_t>(std::lround(alpha))
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

bool point_inside_even_odd(const std::vector<Contour>& contours, const math::Vector2& point) {
    bool inside = false;
    for (const Contour& contour : contours) {
        if (contour.points.size() < 2U) {
            continue;
        }

        for (std::size_t index = 0; index + 1U < contour.points.size(); ++index) {
            const math::Vector2& a = contour.points[index];
            const math::Vector2& b = contour.points[index + 1U];
            const bool crosses = ((a.y > point.y) != (b.y > point.y)) &&
                                 (point.x < (b.x - a.x) * (point.y - a.y) / (b.y - a.y) + a.x);
            if (crosses) {
                inside = !inside;
            }
        }
    }
    return inside;
}

int winding_number(const std::vector<Contour>& contours, const math::Vector2& point) {
    int winding = 0;
    for (const Contour& contour : contours) {
        if (contour.points.size() < 2U) {
            continue;
        }

        for (std::size_t index = 0; index + 1U < contour.points.size(); ++index) {
            const math::Vector2& a = contour.points[index];
            const math::Vector2& b = contour.points[index + 1U];
            if (a.y <= point.y) {
                if (b.y > point.y) {
                    const float cross = (b.x - a.x) * (point.y - a.y) - (point.x - a.x) * (b.y - a.y);
                    if (cross > 0.0f) {
                        ++winding;
                    }
                }
            } else if (b.y <= point.y) {
                const float cross = (b.x - a.x) * (point.y - a.y) - (point.x - a.x) * (b.y - a.y);
                if (cross < 0.0f) {
                    --winding;
                }
            }
        }
    }
    return winding;
}

void rasterize_fill_polygon(SurfaceRGBA& surface, const std::vector<Contour>& contours, const FillPathStyle& style) {
    if (contours.empty()) {
        return;
    }

    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float max_y = std::numeric_limits<float>::lowest();

    for (const Contour& contour : contours) {
        for (const math::Vector2& point : contour.points) {
            min_x = std::min(min_x, point.x);
            min_y = std::min(min_y, point.y);
            max_x = std::max(max_x, point.x);
            max_y = std::max(max_y, point.y);
        }
    }

    const int start_x = std::max(0, static_cast<int>(std::floor(min_x)));
    const int start_y = std::max(0, static_cast<int>(std::floor(min_y)));
    const int end_x = std::min(static_cast<int>(surface.width()), static_cast<int>(std::ceil(max_x)));
    const int end_y = std::min(static_cast<int>(surface.height()), static_cast<int>(std::ceil(max_y)));
    const Color color = apply_opacity(style.fill_color, style.opacity);

    for (int y = start_y; y < end_y; ++y) {
        for (int x = start_x; x < end_x; ++x) {
            const math::Vector2 sample{static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f};
            const bool inside = style.winding == WindingRule::EvenOdd
                ? point_inside_even_odd(contours, sample)
                : winding_number(contours, sample) != 0;
            if (inside) {
                surface.blend_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), color);
            }
        }
    }
}

void rasterize_stroke_polygon(SurfaceRGBA& surface, const std::vector<Contour>& contours, const StrokePathStyle& style) {
    if (contours.empty() || style.stroke_width <= 0.0f) {
        return;
    }

    const float radius = std::max(0.5f, style.stroke_width * 0.5f);
    const float radius_squared = radius * radius;
    const Color color = apply_opacity(style.stroke_color, style.opacity);

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
                const math::Vector2 sample{static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f};
                float min_distance_squared = std::numeric_limits<float>::max();
                for (std::size_t index = 0; index + 1U < contour.points.size(); ++index) {
                    min_distance_squared = std::min(min_distance_squared, distance_to_segment_squared(sample, contour.points[index], contour.points[index + 1U]));
                }
                if (min_distance_squared <= radius_squared) {
                    surface.blend_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), color);
                }
            }
        }
    }
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

} // namespace tachyon::renderer2d
