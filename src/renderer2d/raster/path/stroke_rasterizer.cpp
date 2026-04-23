#include "tachyon/renderer2d/raster/path/stroke_rasterizer.h"
#include "tachyon/renderer2d/raster/path/gradient_lut.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace tachyon::renderer2d {

namespace {

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

float stroke_coverage(const std::vector<Contour>& contours, int x, int y, float radius, const StrokePathStyle& style) {
    (void)style;
    float min_dist_sq = std::numeric_limits<float>::max();
    math::Vector2 p{static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f};
    for (const auto& contour : contours) {
        for (size_t i = 0; i + 1 < contour.points.size(); ++i) {
            min_dist_sq = std::min(min_dist_sq, distance_to_segment_squared(p, contour.points[i].point, contour.points[i + 1].point));
        }
    }
    float dist = std::sqrt(min_dist_sq);
    return std::clamp(radius - dist + 0.5f, 0.0f, 1.0f);
}

} // namespace

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

        for (const auto& point : contour.points) {
            min_x = std::min(min_x, point.point.x);
            min_y = std::min(min_y, point.point.y);
            max_x = std::max(max_x, point.point.x);
            max_y = std::max(max_y, point.point.y);
        }

        const int start_x = std::max(0, static_cast<int>(std::floor(min_x - radius)));
        const int start_y = std::max(0, static_cast<int>(std::floor(min_y - radius)));
        const int end_x = std::min(static_cast<int>(surface.width()), static_cast<int>(std::ceil(max_x + radius)));
        const int end_y = std::min(static_cast<int>(surface.height()), static_cast<int>(std::ceil(max_y + radius)));

        std::optional<GradientLUT> lut;
        if (style.gradient.has_value()) {
            lut.emplace(*style.gradient);
        }

        const bool is_linear = style.gradient.has_value() && style.gradient->type == GradientType::Linear;
        float dt_x = 0.0f;
        if (is_linear) {
            const auto& grad = *style.gradient;
            const math::Vector2 ab = grad.end - grad.start;
            const float len2 = ab.length_squared();
            if (len2 > 1e-6f) {
                dt_x = ab.x / len2;
                // t = (p-start).dot(ab) / len2
                // Compute it per pixel to keep the implementation straightforward.
            }
        }

        for (int y = start_y; y < end_y; ++y) {
            float t_row_base = 0.0f;
            float dt_y = 0.0f;
            if (is_linear) {
                const auto& grad = *style.gradient;
                const math::Vector2 ab = grad.end - grad.start;
                const float len2 = ab.length_squared();
                dt_y = ab.y / len2;
                t_row_base = math::Vector2::dot(math::Vector2(0.5f, (float)y + 0.5f) - grad.start, ab) / len2;
            }

            for (int x = start_x; x < end_x; ++x) {
                const float coverage = stroke_coverage(contours, x, y, radius, style);
                if (coverage > 0.0f) {
                    Color c = style.stroke_color;
                    if (lut) {
                        float cur_t = 0.0f;
                        if (is_linear) {
                            cur_t = t_row_base + (float)x * dt_x;
                        } else {
                            cur_t = (math::Vector2((float)x + 0.5f, (float)y + 0.5f) - style.gradient->start).length() / style.gradient->radial_radius;
                        }
                        c = lut->sample(std::clamp(cur_t, 0.0f, 1.0f));
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

} // namespace tachyon::renderer2d
