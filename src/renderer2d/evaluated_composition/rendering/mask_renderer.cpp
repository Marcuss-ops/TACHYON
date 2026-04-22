#include "tachyon/renderer2d/evaluated_composition/mask_renderer.h"
#include "tachyon/renderer2d/effects/effect_common.h"

#include <algorithm>
#include <cmath>

namespace tachyon::renderer2d {

namespace {

PathGeometry build_mask_geometry(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedShapePath& sp,
    const scene::EvaluatedCompositionState& state,
    RenderContext2D& context) {
    
    PathGeometry geom;
    if (sp.points.empty()) return geom;

    auto apply_tracking = [&](const math::Vector2& lp) -> math::Vector2 {
        if (layer.tracking.mode == "none") return lp;
        
        if (layer.tracking.mode == "point" && layer.tracking.transform_matrix.size() >= 6) {
            // 2x3 Affine transform
            const auto& m = layer.tracking.transform_matrix;
            return {
                lp.x * m[0] + lp.y * m[1] + m[2],
                lp.x * m[3] + lp.y * m[4] + m[5]
            };
        } else if (layer.tracking.mode == "planar" && layer.tracking.transform_matrix.size() >= 9) {
            // 3x3 Homography
            const auto& m = layer.tracking.transform_matrix;
            float w = lp.x * m[6] + lp.y * m[7] + m[8];
            if (std::abs(w) < 1e-7f) w = 1.0f;
            return {
                (lp.x * m[0] + lp.y * m[1] + m[2]) / w,
                (lp.x * m[3] + lp.y * m[4] + m[5]) / w
            };
        }
        return lp;
    };

    auto transform_pt = [&](const math::Vector2& lp) -> math::Vector2 {
        math::Vector2 tracked_lp = apply_tracking(lp);
        math::Vector3 wp = layer.world_matrix.transform_point({tracked_lp.x, tracked_lp.y, 0.0f});
        return {wp.x * context.policy.resolution_scale, wp.y * context.policy.resolution_scale};
    };

    geom.commands.push_back({PathVerb::MoveTo, transform_pt(sp.points[0].position)});
    for (std::size_t i = 1; i < sp.points.size(); ++i) {
        const auto& prev = sp.points[i - 1];
        const auto& curr = sp.points[i];
        if (prev.tangent_out.length_squared() > 1e-6f || curr.tangent_in.length_squared() > 1e-6f) {
            geom.commands.push_back({
                PathVerb::CubicTo,
                transform_pt(prev.position + prev.tangent_out),
                transform_pt(curr.position + curr.tangent_in),
                transform_pt(curr.position)
            });
        } else {
            geom.commands.push_back({PathVerb::LineTo, transform_pt(curr.position)});
        }
    }
    if (sp.closed) {
        geom.commands.push_back({PathVerb::Close});
    }

    (void)state;
    return geom;
}

RectI layer_rect(const scene::EvaluatedLayerState& layer, std::int64_t comp_width, std::int64_t comp_height);

SurfaceRGBA render_mask_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    RenderContext2D& context) {

    const std::uint32_t w = static_cast<std::uint32_t>(std::max<std::int64_t>(1, static_cast<std::int64_t>(std::lround(static_cast<float>(state.width) * context.policy.resolution_scale))));
    const std::uint32_t h = static_cast<std::uint32_t>(std::max<std::int64_t>(1, static_cast<std::int64_t>(std::lround(static_cast<float>(state.height) * context.policy.resolution_scale))));
    SurfaceRGBA surface(w, h);
    surface.clear(Color::transparent());

    const Color mask_color{0.0f, 0.0f, 0.0f, static_cast<float>(std::clamp(layer.opacity, 0.0, 1.0))};
    if (layer.shape_path.has_value()) {
        const PathGeometry geom = build_mask_geometry(layer, *layer.shape_path, state, context);
        if (!geom.commands.empty()) {
            FillPathStyle style;
            style.fill_color = mask_color;
            style.opacity = 1.0f;
            style.winding = WindingRule::NonZero;
            PathRasterizer::fill(surface, geom, style);
        }
    } else {
        const RectI rect = layer_rect(layer, state.width, state.height);
        surface.fill_rect(rect, mask_color, true);
    }

    const float feather = std::max(0.0f, layer.mask_feather * context.policy.resolution_scale);
    if (feather > 0.0f) {
        return blur_alpha_mask(surface, feather);
    }
    return surface;
}

RectI layer_rect(const scene::EvaluatedLayerState& layer, std::int64_t comp_width, std::int64_t comp_height) {
    const std::int64_t base_width = layer.width > 0 ? layer.width : comp_width;
    const std::int64_t base_height = layer.height > 0 ? layer.height : comp_height;
    const float scale_x = std::max(0.0f, std::abs(layer.local_transform.scale.x));
    const float scale_y = std::max(0.0f, std::abs(layer.local_transform.scale.y));
    const int width = std::max(1, static_cast<int>(std::lround(static_cast<double>(base_width) * static_cast<double>(scale_x))));
    const int height = std::max(1, static_cast<int>(std::lround(static_cast<double>(base_height) * static_cast<double>(scale_y))));
    return RectI{
        static_cast<int>(std::lround(layer.local_transform.position.x)),
        static_cast<int>(std::lround(layer.local_transform.position.y)),
        width,
        height
    };
}

RectI shape_bounds_from_path(const scene::EvaluatedShapePath& path) {
    if (path.points.empty()) {
        return RectI{0, 0, 0, 0};
    }

    float min_x = path.points.front().position.x;
    float min_y = path.points.front().position.y;
    float max_x = path.points.front().position.x;
    float max_y = path.points.front().position.y;
    for (const auto& point : path.points) {
        min_x = std::min(min_x, point.position.x);
        min_y = std::min(min_y, point.position.y);
        max_x = std::max(max_x, point.position.x);
        max_y = std::max(max_y, point.position.y);
    }

    return RectI{
        static_cast<int>(std::floor(min_x)),
        static_cast<int>(std::floor(min_y)),
        std::max(1, static_cast<int>(std::ceil(max_x - min_x))),
        std::max(1, static_cast<int>(std::ceil(max_y - min_y)))
    };
}

void fill_rect_alpha(
    std::vector<float>& accum_a,
    std::int64_t width,
    std::int64_t height,
    const RectI& rect,
    float value) {

    if (rect.width <= 0 || rect.height <= 0 || width <= 0 || height <= 0) {
        return;
    }

    const int x0 = std::max(0, rect.x);
    const int y0 = std::max(0, rect.y);
    const int x1 = std::min(static_cast<int>(width), rect.x + rect.width);
    const int y1 = std::min(static_cast<int>(height), rect.y + rect.height);
    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
            if (index < accum_a.size()) {
                accum_a[index] = std::clamp(value, 0.0f, 1.0f);
            }
        }
    }
}

} // namespace

void MaskRenderer::applyMask(
    const scene::EvaluatedCompositionState& state,
    RenderContext2D& context,
    std::vector<float>& accum_r,
    std::vector<float>& accum_g,
    std::vector<float>& accum_b,
    std::vector<float>& accum_a) {
    const float res_scale = context.policy.resolution_scale;
    const std::uint32_t expected_width = static_cast<std::uint32_t>(std::max<std::int64_t>(1, static_cast<std::int64_t>(std::lround(static_cast<float>(state.width) * res_scale))));
    const std::uint32_t expected_height = static_cast<std::uint32_t>(std::max<std::int64_t>(1, static_cast<std::int64_t>(std::lround(static_cast<float>(state.height) * res_scale))));
    const std::size_t expected_size = static_cast<std::size_t>(expected_width) * static_cast<std::size_t>(expected_height);
    if (accum_a.size() < expected_size || accum_r.size() < expected_size || accum_g.size() < expected_size || accum_b.size() < expected_size) {
        return;
    }

    std::vector<float> mask_alpha(expected_size, 1.0f);
    bool found_mask = false;
    for (const auto& layer : state.layers) {
        if (layer.type != scene::LayerType::Mask || !layer.enabled || !layer.active) {
            continue;
        }
        found_mask = true;
        const SurfaceRGBA mask_surface = render_mask_surface(layer, state, context);
        const std::uint32_t width = std::min(expected_width, mask_surface.width());
        const std::uint32_t height = std::min(expected_height, mask_surface.height());
        for (std::uint32_t y = 0; y < height; ++y) {
            for (std::uint32_t x = 0; x < width; ++x) {
                const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(expected_width) + static_cast<std::size_t>(x);
                if (index < mask_alpha.size()) {
                    mask_alpha[index] = mask_surface.get_pixel(x, y).a;
                }
            }
        }
    }

    if (!found_mask) {
        return;
    }

    for (std::size_t index = 0; index < expected_size; ++index) {
        const float factor = std::clamp(mask_alpha[index], 0.0f, 1.0f);
        accum_r[index] *= factor;
        accum_g[index] *= factor;
        accum_b[index] *= factor;
        accum_a[index] *= factor;
    }
}

void MaskRenderer::computeMaskBounds(
    const scene::EvaluatedLayerState& layer_state,
    int& x0, int& y0, int& x1, int& y1) {
    (void)layer_state;
    x0 = y0 = x1 = y1 = 0;
}

} // namespace tachyon::renderer2d
