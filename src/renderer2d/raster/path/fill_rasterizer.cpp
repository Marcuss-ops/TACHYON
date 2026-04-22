#include "tachyon/renderer2d/raster/path/fill_rasterizer.h"
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

} // namespace

void rasterize_fill_polygon(SurfaceRGBA& surface, const std::vector<Contour>& contours, const FillPathStyle& style) {
    if (contours.empty()) return;

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

    std::vector<float> coverage_buffer(width + 1, 0.0f);

    for (int y = start_y; y < end_y; ++y) {
        std::fill(coverage_buffer.begin(), coverage_buffer.end(), 0.0f);

        for (const auto& contour : contours) {
            for (size_t i = 0; i + 1 < contour.points.size(); ++i) {
                const auto& p1 = contour.points[i];
                const auto& p2 = contour.points[i + 1];

                float y1 = p1.y, y2 = p2.y;
                if (std::max(y1, y2) <= (float)y || std::min(y1, y2) >= (float)y + 1) continue;

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
                    const float area = height * (1.0f - (x1 + x2) * 0.5f + static_cast<float>(ix1));
                    if (ix1 >= 0 && ix1 < width) coverage_buffer[ix1] += area;
                } else {
                    const float slope = (x2 - x1) / (sy2 - sy1);
                    const float dir = (y2 > y1) ? 1.0f : -1.0f;
                    
                    float next_ix_x = static_cast<float>(ix1 + 1);
                    float next_sy = sy1 + (next_ix_x - x1) / slope;
                    float h = next_sy - sy1;
                    coverage_buffer[std::clamp(ix1, 0, width)] += h * dir * (1.0f - (x1 + next_ix_x) * 0.5f + static_cast<float>(ix1));
                    
                    for (int ix = ix1 + 1; ix < ix2; ++ix) {
                        float cur_sy = next_sy;
                        next_ix_x = static_cast<float>(ix + 1);
                        next_sy = sy1 + (next_ix_x - x1) / slope;
                        h = next_sy - cur_sy;
                        coverage_buffer[std::clamp(ix, 0, width)] += h * dir * 0.5f;
                    }
                    
                    float final_h = sy2 - next_sy;
                    coverage_buffer[std::clamp(ix2, 0, width)] += final_h * dir * (1.0f - (next_ix_x + x2) * 0.5f + static_cast<float>(ix2));
                }

                const int next_x = std::max(0, ix2 + 1);
                if (next_x < width) coverage_buffer[next_x] += height;
            }
        }

        float winding = 0.0f;
        for (int x = 0; x < width; ++x) {
            winding += coverage_buffer[x];
            float coverage = 0.0f;
            if (style.winding == WindingRule::NonZero) {
                coverage = std::clamp(std::abs(winding), 0.0f, 1.0f);
            } else {
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

} // namespace tachyon::renderer2d
