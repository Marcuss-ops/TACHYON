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
                    
                    float final_a = c.a * style.opacity * coverage;
                    float* p = &surface.mutable_pixels()[(static_cast<size_t>(y) * width + x) * 4];
                    float inv_a = 1.0f - final_a;
                    p[0] = c.r * final_a + p[0] * inv_a;
                    p[1] = c.g * final_a + p[1] * inv_a;
                    p[2] = c.b * final_a + p[2] * inv_a;
                    p[3] = final_a + p[3] * inv_a;
                }
            }
        }
        return;
    }

    // Non-feathered path (original algorithm)
    for (int y = start_y; y < end_y; ++y) {
        std::fill(coverage_buffer.begin(), coverage_buffer.end(), 0.0f);

        for (const auto& contour : contours) {
            for (size_t i = 0; i + 1 < contour.points.size(); ++i) {
                const auto& p1 = contour.points[i].point;
                const auto& p2 = contour.points[i + 1].point;

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
        float* row_ptr = &surface.mutable_pixels()[(static_cast<size_t>(y) * width) * 4];

        float t = 0.0f;
        float dt = 0.0f;
        bool is_linear = false;
        if (style.gradient.has_value() && style.gradient->type == GradientType::Linear) {
            const auto& grad = *style.gradient;
            const math::Vector2 ab = grad.end - grad.start;
            const float len2 = ab.length_squared();
            if (len2 > 1e-6f) {
                is_linear = true;
                dt = ab.x / len2;
                t = math::Vector2::dot(math::Vector2(0.0f, (float)y) - grad.start, ab) / len2;
            }
        }

#ifdef TACHYON_AVX2
        int x = 0;
        // AVX2 Winding loop
        __m256 v_winding = _mm256_set1_ps(0.0f);
        __m256 v_one = _mm256_set1_ps(1.0f);
        __m256 v_zero = _mm256_set1_ps(0.0f);
        __m256 v_two = _mm256_set1_ps(2.0f);
        // __m256 v_opacity = _mm256_set1_ps(style.opacity); // Not used

        for (; x <= width - 8; x += 8) {
            __m256 v_cov = _mm256_loadu_ps(&coverage_buffer[x]);
            
            v_cov = _mm256_add_ps(v_cov, _mm256_castsi256_ps(_mm256_slli_si256(_mm256_castps_si256(v_cov), 4)));
            v_cov = _mm256_add_ps(v_cov, _mm256_castsi256_ps(_mm256_slli_si256(_mm256_castps_si256(v_cov), 8)));
            __m256 v_low = _mm256_permute2f128_ps(v_cov, v_cov, 0x00);
            __m256 v_high = _mm256_permute2f128_ps(v_cov, v_cov, 0x11);
            __m256 v_last_low = _mm256_shuffle_ps(v_low, v_low, _MM_SHUFFLE(3, 3, 3, 3));
            v_high = _mm256_add_ps(v_high, v_last_low);
            v_cov = _mm256_blend_ps(v_low, v_high, 0xF0);
            
            __m256 v_curr_winding = _mm256_add_ps(v_cov, v_winding);
            
            __m128 v_upper = _mm256_extractf128_ps(v_curr_winding, 1);
            float last_w = _mm_cvtss_f32(_mm_shuffle_ps(v_upper, v_upper, _MM_SHUFFLE(3, 3, 3, 3)));
            v_winding = _mm256_set1_ps(last_w);

            __m256 v_abs_w = _mm256_andnot_ps(_mm256_set1_ps(-0.0f), v_curr_winding);
            __m256 v_coverage;
            if (style.winding == WindingRule::NonZero) {
                v_coverage = _mm256_min_ps(v_one, v_abs_w);
            } else {
                __m256 v_w_div_2 = _mm256_mul_ps(v_abs_w, _mm256_set1_ps(0.5f));
                __m256 v_w_floor = _mm256_floor_ps(v_w_div_2);
                __m256 v_rem = _mm256_sub_ps(v_abs_w, _mm256_mul_ps(v_w_floor, v_two));
                v_coverage = _mm256_blendv_ps(v_rem, _mm256_sub_ps(v_two, v_rem), _mm256_cmp_ps(v_rem, v_one, _CMP_GT_OQ));
            }

            __m256 v_mask = _mm256_cmp_ps(v_coverage, _mm256_set1_ps(0.001f), _CMP_GT_OQ);
            if (_mm256_movemask_ps(v_mask) == 0) continue;

            alignas(32) float covs[8];
            _mm256_store_ps(covs, v_coverage);
            
            for (int j = 0; j < 8; ++j) {
                if (covs[j] > 0.001f) {
                    int cur_x = x + j;
                    Color c = style.fill_color;
                    if (lut) {
                        float cur_t = is_linear ? std::clamp(t + (cur_x * dt), 0.0f, 1.0f) : std::clamp((math::Vector2((float)cur_x, (float)y) - style.gradient->start).length() / style.gradient->radial_radius, 0.0f, 1.0f);
                        c = lut->sample(cur_t);
                    }
                    
                    float final_a = c.a * style.opacity * covs[j];
                    float* p = &row_ptr[cur_x * 4];
                    float inv_a = 1.0f - final_a;
                    p[0] = c.r * final_a + p[0] * inv_a;
                    p[1] = c.g * final_a + p[1] * inv_a;
                    p[2] = c.b * final_a + p[2] * inv_a;
                    p[3] = final_a + p[3] * inv_a;
                }
            }
        }
#else
        int x = 0;
#endif

        // Tail loop
        for (; x < width; ++x) {
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
                if (lut) {
                    float cur_t = is_linear ? std::clamp(t + (x * dt), 0.0f, 1.0f) : std::clamp((math::Vector2((float)x, (float)y) - style.gradient->start).length() / style.gradient->radial_radius, 0.0f, 1.0f);
                    c = lut->sample(cur_t);
                }
                
                float final_a = c.a * style.opacity * coverage;
                float* p = &row_ptr[x * 4];
                float inv_a = 1.0f - final_a;
                p[0] = c.r * final_a + p[0] * inv_a;
                p[1] = c.g * final_a + p[1] * inv_a;
                p[2] = c.b * final_a + p[2] * inv_a;
                p[3] = final_a + p[3] * inv_a;
            }
        }
    }
}

} // namespace tachyon::renderer2d
