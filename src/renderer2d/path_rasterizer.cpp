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

bool point_inside_non_zero(const std::vector<Contour>& contours, const math::Vector2& point) {
    return winding_number(contours, point) != 0;
}

float fill_coverage(const std::vector<Contour>& contours, int x, int y, WindingRule winding) {
    int covered_samples = 0;
    const float inv_samples = 1.0f / static_cast<float>(kCoverageSamples * kCoverageSamples);

    for (int sample_y = 0; sample_y < kCoverageSamples; ++sample_y) {
        for (int sample_x = 0; sample_x < kCoverageSamples; ++sample_x) {
            const float px = static_cast<float>(x) + (static_cast<float>(sample_x) + 0.5f) / static_cast<float>(kCoverageSamples);
            const float py = static_cast<float>(y) + (static_cast<float>(sample_y) + 0.5f) / static_cast<float>(kCoverageSamples);
            const math::Vector2 sample{px, py};
            const bool inside = winding == WindingRule::EvenOdd
                ? point_inside_even_odd(contours, sample)
                : point_inside_non_zero(contours, sample);
            if (inside) {
                ++covered_samples;
            }
        }
    }

    return static_cast<float>(covered_samples) * inv_samples;
}

float stroke_coverage(const std::vector<Contour>& contours, int x, int y, float radius, const StrokePathStyle& style) {
    if (radius <= 0.0f) {
        return 0.0f;
    }

    const float radius_squared = radius * radius;
    int covered_samples = 0;
    const float inv_samples = 1.0f / static_cast<float>(kCoverageSamples * kCoverageSamples);

    for (int sample_y = 0; sample_y < kCoverageSamples; ++sample_y) {
        for (int sample_x = 0; sample_x < kCoverageSamples; ++sample_x) {
            const math::Vector2 sample{
                static_cast<float>(x) + (static_cast<float>(sample_x) + 0.5f) / static_cast<float>(kCoverageSamples),
                static_cast<float>(y) + (static_cast<float>(sample_y) + 0.5f) / static_cast<float>(kCoverageSamples)
            };

            bool covered = false;
            for (const Contour& contour : contours) {
                if (contour.points.size() < 2U) continue;

                // 1. Check segments and joints
                for (std::size_t i = 0; i + 1U < contour.points.size(); ++i) {
                    const math::Vector2& a = contour.points[i];
                    const math::Vector2& b = contour.points[i+1];
                    const math::Vector2 ab = b - a;
                    const float len2 = ab.length_squared();
                    if (len2 < 1e-6f) continue;

                    const float t = math::Vector2::dot(sample - a, ab) / len2;
                    
                    // Cap/Join Clipping
                    float t_min = 0.0f;
                    float t_max = 1.0f;
                    
                    // At start of contour
                    if (i == 0) {
                        if (style.cap == LineCap::Square) t_min = -radius / std::sqrt(len2);
                        // Round/Butt handle via distance below
                    }
                    
                    // At end of contour
                    if (i + 2 == contour.points.size()) {
                        if (style.cap == LineCap::Square) t_max = 1.0f + radius / std::sqrt(len2);
                    }

                    if (t >= t_min && t <= t_max) {
                        const math::Vector2 projection = a + ab * std::clamp(t, 0.0f, 1.0f);
                        if ((sample - projection).length_squared() <= radius_squared) {
                            covered = true;
                            break;
                        }
                    }

                    // Handle Round Caps (if not clipped by Butt/Square logic)
                    if (style.cap == LineCap::Round) {
                        if (i == 0 && (sample - a).length_squared() <= radius_squared) {
                            covered = true;
                            break;
                        }
                        if (i + 2 == contour.points.size() && (sample - b).length_squared() <= radius_squared) {
                            covered = true;
                            break;
                        }
                    }

                    // Handle Joins
                    if (i + 2 < contour.points.size()) {
                        const math::Vector2& next_c = contour.points[i+2];
                        const math::Vector2 bc = next_c - b;
                        const float len_bc2 = bc.length_squared();
                        
                        if (len_bc2 > 1e-6f) {
                            if (style.join == LineJoin::Round) {
                                if ((sample - b).length_squared() <= radius_squared) {
                                    covered = true;
                                    break;
                                }
                            } else {
                                // Miter/Bevel Join logic
                                const math::Vector2 v1 = ab / std::sqrt(len2);
                                const math::Vector2 v2 = bc / std::sqrt(len_bc2);
                                const math::Vector2 n1{-v1.y, v1.x};
                                const math::Vector2 n2{-v2.y, v2.x};
                                
                                // Determine the "outer" side based on winding
                                float cross = v1.x * v2.y - v1.y * v2.x;
                                float side = (cross > 0) ? 1.0f : -1.0f;
                                
                                const math::Vector2 miter_dir = (n1 + n2);
                                const float miter_len2 = miter_dir.length_squared();
                                
                                if (miter_len2 > 1e-6f) {
                                    const math::Vector2 miter_vec = miter_dir * (2.0f * radius / miter_len2);
                                    const float miter_scale = std::sqrt(miter_len2) / 2.0f; // cos(theta/2) equivalent
                                    
                                    // Check if point is inside the join wedge
                                    // Point must be on the outer side and within the miter area
                                    const math::Vector2 rel = sample - b;
                                    const float d1 = math::Vector2::dot(rel, n1 * side);
                                    const float d2 = math::Vector2::dot(rel, n2 * side);
                                    
                                    if (d1 > 0 && d2 > 0) {
                                        if (style.join == LineJoin::Miter && (miter_scale > 0 && 1.0f / miter_scale <= style.miter_limit)) {
                                            // Inside miter: point must be within both offset planes
                                            if (d1 <= radius && d2 <= radius) {
                                                covered = true;
                                                break;
                                            }
                                        } else {
                                            // Bevel/Miter fallback: point must be within the bevel triangle
                                            // Triangle: B, B + n1*r*side, B + n2*r*side
                                            const float d_bevel = math::Vector2::dot(rel, v2 - v1);
                                            if (d1 <= radius && d2 <= radius && d_bevel <= 0) {
                                                covered = true;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                if (covered) break;
            }

            if (covered) {
                ++covered_samples;
            }
        }
    }

    return static_cast<float>(covered_samples) * inv_samples;
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

    for (int y = start_y; y < end_y; ++y) {
        for (int x = start_x; x < end_x; ++x) {
            const float coverage = fill_coverage(contours, x, y, style.winding);
            if (coverage > 0.0f) {
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

void PathRasterizer::fill(SurfaceRGBA& surface, const PathGeometry& path, const FillPathStyle& style) {
    const std::vector<Contour> contours = build_contours(path);
    rasterize_fill_polygon(surface, contours, style);
}

void PathRasterizer::stroke(SurfaceRGBA& surface, const PathGeometry& path, const StrokePathStyle& style) {
    const std::vector<Contour> contours = build_contours(path);
    rasterize_stroke_polygon(surface, contours, style);
}

} // namespace tachyon::renderer2d
