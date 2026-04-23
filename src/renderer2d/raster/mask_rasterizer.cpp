#include "tachyon/renderer2d/raster/mask_rasterizer.h"
#include "tachyon/renderer2d/raster/path/path_flattener.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace tachyon::renderer2d {

namespace {

constexpr float kDistanceEpsilon = 1e-6f;

float point_segment_distance(const math::Vector2& p, const math::Vector2& a, const math::Vector2& b) {
    const math::Vector2 ab = b - a;
    const float len2 = ab.length_squared();
    if (len2 <= kDistanceEpsilon) {
        return (p - a).length();
    }

    const float t = std::clamp(math::Vector2::dot(p - a, ab) / len2, 0.0f, 1.0f);
    const math::Vector2 closest = a + ab * t;
    return (p - closest).length();
}

int winding_count_at_point(const math::Vector2& p, const Contour& contour) {
    int winding_count = 0;
    const std::size_t count = contour.points.size();
    if (count < 2U) {
        return 0;
    }

    for (std::size_t i = 0; i < count; ++i) {
        const auto& a = contour.points[i].point;
        const auto& b = contour.points[(i + 1U) % count].point;
        if (((a.y > p.y) != (b.y > p.y)) &&
            (p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y + kDistanceEpsilon) + a.x)) {
            winding_count += (b.y > a.y) ? 1 : -1;
        }
    }

    return winding_count;
}

void nearest_point_on_contour(
    const math::Vector2& p,
    const Contour& contour,
    math::Vector2& out_point,
    float& out_feather_inner,
    float& out_feather_outer,
    float& out_dist) {

    const std::size_t count = contour.points.size();
    if (count < 2U) {
        out_dist = std::numeric_limits<float>::max();
        out_point = {};
        out_feather_inner = 0.0f;
        out_feather_outer = 0.0f;
        return;
    }

    float best_dist = std::numeric_limits<float>::max();
    std::size_t best_index = 0U;
    float best_t = 0.0f;
    math::Vector2 best_point{};

    for (std::size_t i = 0; i < count; ++i) {
        const auto& a = contour.points[i];
        const auto& b = contour.points[(i + 1U) % count];
        const math::Vector2 ab = b.point - a.point;
        const float len2 = ab.length_squared();

        float t = 0.0f;
        if (len2 > kDistanceEpsilon) {
            t = std::clamp(math::Vector2::dot(p - a.point, ab) / len2, 0.0f, 1.0f);
        }

        const math::Vector2 closest = a.point + ab * t;
        const float dist = (p - closest).length();
        if (dist < best_dist) {
            best_dist = dist;
            best_index = i;
            best_t = t;
            best_point = closest;
        }
    }

    const auto& a = contour.points[best_index];
    const auto& b = contour.points[(best_index + 1U) % count];
    out_point = best_point;
    out_feather_inner = a.feather_inner * (1.0f - best_t) + b.feather_inner * best_t;
    out_feather_outer = a.feather_outer * (1.0f - best_t) + b.feather_outer * best_t;
    out_dist = best_dist;
}

float sample_contour_alpha(
    const math::Vector2& p,
    const Contour& contour,
    WindingRule winding) {

    if (contour.points.size() < 2U) {
        return 0.0f;
    }

    int winding_count = winding_count_at_point(p, contour);
    const bool inside = (winding == WindingRule::NonZero)
        ? (winding_count != 0)
        : ((std::abs(winding_count) % 2) != 0);

    math::Vector2 nearest_point{};
    float feather_inner = 0.0f;
    float feather_outer = 0.0f;
    float edge_dist = 0.0f;
    nearest_point_on_contour(p, contour, nearest_point, feather_inner, feather_outer, edge_dist);

    const float inner_expand = std::max(0.0f, -feather_inner);
    if (inside) {
        if (inner_expand > kDistanceEpsilon && edge_dist < inner_expand) {
            return std::clamp(0.5f + 0.5f * (edge_dist / inner_expand), 0.0f, 1.0f);
        }
        return 1.0f;
    }

    if (feather_outer > kDistanceEpsilon && edge_dist < feather_outer) {
        return std::clamp(0.5f * (1.0f - edge_dist / feather_outer), 0.0f, 1.0f);
    }

    return 0.0f;
}

} // namespace

void MaskRasterizer::rasterize(
    int width, int height,
    const std::vector<Contour>& contours,
    const Config& cfg,
    float* out_alpha) {

    if (!out_alpha || width <= 0 || height <= 0) {
        return;
    }

    const std::size_t pixel_count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    std::fill(out_alpha, out_alpha + pixel_count, 0.0f);

    if (contours.empty()) {
        return;
    }

    constexpr int kSamplesPerAxis = 4;
    constexpr float kInvSampleCount = 1.0f / static_cast<float>(kSamplesPerAxis * kSamplesPerAxis);
    const float sample_step = 1.0f / static_cast<float>(kSamplesPerAxis);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float coverage = 0.0f;

            for (int sy = 0; sy < kSamplesPerAxis; ++sy) {
                for (int sx = 0; sx < kSamplesPerAxis; ++sx) {
                    const math::Vector2 sample{
                        static_cast<float>(x) + (static_cast<float>(sx) + 0.5f) * sample_step,
                        static_cast<float>(y) + (static_cast<float>(sy) + 0.5f) * sample_step
                    };

                    float sample_alpha = 0.0f;
                    for (const auto& contour : contours) {
                        const float contour_alpha = sample_contour_alpha(sample, contour, cfg.winding);
                        sample_alpha = sample_alpha + contour_alpha * (1.0f - sample_alpha);
                    }

                    coverage += sample_alpha;
                }
            }

            out_alpha[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)] =
                std::clamp(coverage * kInvSampleCount, 0.0f, 1.0f);
        }
    }
}

} // namespace tachyon::renderer2d
