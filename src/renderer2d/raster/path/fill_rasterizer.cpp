#include "tachyon/renderer2d/raster/path/fill_rasterizer.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <immintrin.h>

namespace tachyon::renderer2d {

namespace {

inline Color apply_coverage(Color color, float opacity, float coverage) {
    const float alpha = color.a * std::clamp(opacity, 0.0f, 1.0f) * std::clamp(coverage, 0.0f, 1.0f);
    color.a = alpha;
    return color;
}

// Compute feather coverage based on distance from edge
// For a point at (x, y), compute distance to the nearest edge of the contour
// If inside the shape: coverage decreases from 1.0 to 0.0 as we approach outer feather boundary
// If outside the shape: coverage increases from 0.0 to 1.0 as we enter the inner feather zone
float compute_feather_coverage(float signed_distance, float feather_inner, float feather_outer) {
    if (signed_distance > 0.0f) {
        // Outside the shape - apply outer feather
        if (feather_outer <= 0.0f) return 0.0f;
        return std::clamp(1.0f - (signed_distance / feather_outer), 0.0f, 1.0f);
    } else {
        // Inside the shape - apply inner feather (expansion)
        if (feather_inner >= 0.0f) return 1.0f;
        float dist = std::abs(signed_distance);
        return std::clamp(1.0f - (dist / std::abs(feather_inner)), 0.0f, 1.0f);
    }
}

// Simple distance from point to line segment
float point_to_segment_distance(const math::Vector2& p, const math::Vector2& a, const math::Vector2& b) {
    math::Vector2 ab = b - a;
    math::Vector2 ap = p - a;
    float t = math::Vector2::dot(ap, ab) / (ab.length_squared() + 1e-6f);
    t = std::clamp(t, 0.0f, 1.0f);
    math::Vector2 projection = a + ab * t;
    return (p - projection).length();
}

// Compute signed distance from point to contour (negative = inside, positive = outside)
float signed_distance_to_contour(const math::Vector2& p, const Contour& contour) {
    if (contour.points.size() < 2) return 1e6f;
    
    float min_dist = 1e6f;
    int winding = 0;
    
    for (size_t i = 0; i + 1 < contour.points.size(); ++i) {
        const auto& a = contour.points[i].point;
        const auto& b = contour.points[i + 1].point;
        
        float dist = point_to_segment_distance(p, a, b);
        min_dist = std::min(min_dist, dist);
        
        // Ray casting for winding
        if (((a.y > p.y) != (b.y > p.y)) && (p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x)) {
            winding = (winding + 1) % 2;
        }
    }
    
    return winding ? -min_dist : min_dist;
}

int winding_count_at_point(const math::Vector2& p, const Contour& contour) {
    int winding_count = 0;
    const std::size_t n = contour.points.size();
    for (std::size_t i = 0; i < n; ++i) {
        const auto& a = contour.points[i].point;
        const auto& b = contour.points[(i + 1) % n].point;
        if (((a.y > p.y) != (b.y > p.y)) &&
            (p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y + 1e-12f) + a.x)) {
            winding_count += (b.y > a.y) ? 1 : -1;
        }
    }
    return winding_count;
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

    // Check if any contour has feather
    bool has_feather = false;
    for (const auto& contour : contours) {
        for (const auto& pt : contour.points) {
            if (std::abs(pt.feather_inner) > 1e-6f || std::abs(pt.feather_outer) > 1e-6f) {
                has_feather = true;
                break;
            }
        }
        if (has_feather) break;
    }

    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float max_y = std::numeric_limits<float>::lowest();

    for (const auto& contour : contours) {
        for (const auto& p : contour.points) {
            min_x = std::min(min_x, p.point.x);
            min_y = std::min(min_y, p.point.y);
            max_x = std::max(max_x, p.point.x);
            max_y = std::max(max_y, p.point.y);
        }
    }

    const int start_y = std::max(0, static_cast<int>(std::floor(min_y)));
    const int end_y = std::min(static_cast<int>(surface.height()), static_cast<int>(std::ceil(max_y)));
    const int width = static_cast<int>(surface.width());

    std::vector<float> coverage_buffer(width + 1, 0.0f);
    std::optional<GradientLUT> lut;
    if (style.gradient.has_value()) {
        lut.emplace(*style.gradient);
    }

    // For feathered rendering, we need to compute per-pixel distance
    if (has_feather) {
        for (int y = start_y; y < end_y; ++y) {
            for (int x = 0; x < width; ++x) {
                math::Vector2 pixel_pos{(float)x + 0.5f, (float)y + 0.5f};
                
                // Find nearest contour and compute signed distance
                float min_signed_dist = 1e6f;
                float feather_inner = 0.0f;
                float feather_outer = 0.0f;
                
                for (const auto& contour : contours) {
                    float dist = signed_distance_to_contour(pixel_pos, contour);
                    if (std::abs(dist) < std::abs(min_signed_dist)) {
                        min_signed_dist = dist;
                        // Use feather from nearest point on contour
                        // For simplicity, use the first point's feather values
                        if (!contour.points.empty()) {
                            feather_inner = contour.points[0].feather_inner;
                            feather_outer = contour.points[0].feather_outer;
                        }
                    }
                }
                
                float coverage = compute_feather_coverage(min_signed_dist, feather_inner, feather_outer);
                
                if (coverage > 0.001f) {
                    Color c = style.fill_color;
                    if (lut) {
                        float t = 0.0f;
                        if (style.gradient->type == GradientType::Linear) {
                            const auto& grad = *style.gradient;
                            math::Vector2 ab = grad.end - grad.start;
                            float len2 = ab.length_squared();
                            if (len2 > 1e-6f) {
                                t = std::clamp(math::Vector2::dot(pixel_pos - grad.start, ab) / len2, 0.0f, 1.0f);
                            }
                        } else {
                            t = std::clamp((pixel_pos - style.gradient->start).length() / style.gradient->radial_radius, 0.0f, 1.0f);
                        }
                        c = lut->sample(t);
                    }
                    
                    const float final_a = c.a * style.opacity * coverage;
                    surface.blend_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), c, final_a);
                }
            }
        }
        return;
    }

    // Non-feathered path: supersample each pixel for stable AA coverage.
    constexpr int kSamplesPerAxis = 4;
    constexpr float kInvSampleCount = 1.0f / static_cast<float>(kSamplesPerAxis * kSamplesPerAxis);
    const float sample_step = 1.0f / static_cast<float>(kSamplesPerAxis);

    for (int y = start_y; y < end_y; ++y) {
        for (int x = 0; x < width; ++x) {
            float coverage = 0.0f;
            for (int sy = 0; sy < kSamplesPerAxis; ++sy) {
                for (int sx = 0; sx < kSamplesPerAxis; ++sx) {
                    const math::Vector2 sample{
                        static_cast<float>(x) + (static_cast<float>(sx) + 0.5f) * sample_step,
                        static_cast<float>(y) + (static_cast<float>(sy) + 0.5f) * sample_step
                    };

                    int total_winding = 0;
                    for (const auto& contour : contours) {
                        total_winding += winding_count_at_point(sample, contour);
                    }

                    const bool inside = (style.winding == WindingRule::NonZero)
                        ? (total_winding != 0)
                        : ((std::abs(total_winding) % 2) != 0);
                    coverage += inside ? 1.0f : 0.0f;
                }
            }

            coverage *= kInvSampleCount;
            if (coverage <= 0.001f) {
                continue;
            }

            Color c = style.fill_color;
            if (lut) {
                const math::Vector2 pixel_pos{static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f};
                float cur_t = 0.0f;
                if (style.gradient->type == GradientType::Linear) {
                    const auto& grad = *style.gradient;
                    const math::Vector2 ab = grad.end - grad.start;
                    const float len2 = ab.length_squared();
                    if (len2 > 1e-6f) {
                        cur_t = std::clamp(math::Vector2::dot(pixel_pos - grad.start, ab) / len2, 0.0f, 1.0f);
                    }
                } else {
                    const auto& grad = *style.gradient;
                    cur_t = std::clamp((pixel_pos - grad.start).length() / grad.radial_radius, 0.0f, 1.0f);
                }
                c = lut->sample(cur_t);
            }

            const float final_a = c.a * style.opacity * coverage;
            surface.blend_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), c, final_a);
        }
    }
}

} // namespace tachyon::renderer2d
