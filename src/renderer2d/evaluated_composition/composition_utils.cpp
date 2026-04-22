#include "composition_utils.h"
#include "tachyon/renderer2d/color/blending.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include <algorithm>
#include <cmath>

namespace tachyon {

renderer2d::Color from_color_spec(const ColorSpec& spec) {
    return renderer2d::detail::sRGB_to_Linear(renderer2d::Color{
        static_cast<float>(spec.r) / 255.0f,
        static_cast<float>(spec.g) / 255.0f,
        static_cast<float>(spec.b) / 255.0f,
        static_cast<float>(spec.a) / 255.0f
    });
}

renderer2d::Color apply_opacity(renderer2d::Color color, double opacity) {
    color.a *= static_cast<float>(std::clamp(opacity, 0.0, 1.0));
    return color;
}

renderer2d::RectI layer_rect(const scene::EvaluatedLayerState& layer, std::int64_t comp_width, std::int64_t comp_height, float resolution_scale) {
    const std::int64_t base_width = layer.width > 0 ? layer.width : comp_width;
    const std::int64_t base_height = layer.height > 0 ? layer.height : comp_height;
    const float scale_x = std::max(0.0f, std::abs(layer.local_transform.scale.x)) * resolution_scale;
    const float scale_y = std::max(0.0f, std::abs(layer.local_transform.scale.y)) * resolution_scale;
    const int width = std::max(1, static_cast<int>(std::lround(static_cast<double>(base_width) * static_cast<double>(scale_x))));
    const int height = std::max(1, static_cast<int>(std::lround(static_cast<double>(base_height) * static_cast<double>(scale_y))));
    return renderer2d::RectI{
        static_cast<int>(std::lround(layer.local_transform.position.x * resolution_scale)),
        static_cast<int>(std::lround(layer.local_transform.position.y * resolution_scale)),
        width,
        height
    };
}

renderer2d::RectI shape_bounds_from_path(const scene::EvaluatedShapePath& path) {
    if (path.points.empty()) {
        return renderer2d::RectI{0, 0, 0, 0};
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

    return renderer2d::RectI{
        static_cast<int>(std::floor(min_x)),
        static_cast<int>(std::floor(min_y)),
        std::max(1, static_cast<int>(std::ceil(max_x - min_x))),
        std::max(1, static_cast<int>(std::ceil(max_y - min_y)))
    };
}

std::shared_ptr<renderer2d::SurfaceRGBA> make_surface(std::int64_t width, std::int64_t height, renderer2d::RenderContext2D& context) {
    const std::uint32_t w = static_cast<std::uint32_t>(std::max<std::int64_t>(1, width));
    const std::uint32_t h = static_cast<std::uint32_t>(std::max<std::int64_t>(1, height));
    
    if (context.surface_pool) {
        auto surface = context.surface_pool->acquire(w, h);
        surface->clear(renderer2d::Color::transparent());
        return surface;
    }

    return std::make_shared<renderer2d::SurfaceRGBA>(w, h);
}

void multiply_surface_alpha(renderer2d::SurfaceRGBA& surface, float factor) {
    const float clamped = std::clamp(factor, 0.0f, 1.0f);
    if (clamped >= 0.9999f) {
        return;
    }

    for (std::uint32_t y = 0; y < surface.height(); ++y) {
        for (std::uint32_t x = 0; x < surface.width(); ++x) {
            renderer2d::Color px = surface.get_pixel(x, y);
            if (px.a <= 0.0f) {
                continue;
            }
            px.r *= clamped;
            px.g *= clamped;
            px.b *= clamped;
            px.a *= clamped;
            surface.set_pixel(x, y, px);
        }
    }
}

void apply_mask(renderer2d::SurfaceRGBA& surface, const renderer2d::SurfaceRGBA& mask, TrackMatteType type) {
    if (type == TrackMatteType::None) return;
    
    const std::uint32_t width = std::min(surface.width(), mask.width());
    const std::uint32_t height = std::min(surface.height(), mask.height());
    
    auto& s_pixels = surface.mutable_pixels();
    const auto& m_pixels = mask.pixels();
    
    const std::uint32_t s_w = surface.width();
    const std::uint32_t m_w = mask.width();

    for (std::uint32_t y = 0; y < height; ++y) {
        float* s_row = &s_pixels[y * s_w * 4];
        const float* m_row = &m_pixels[y * m_w * 4];
        
        for (std::uint32_t x = 0; x < width; ++x) {
            const float* m = &m_row[x * 4];
            float* s = &s_row[x * 4];
            
            // Rec.709 Luma weights: 0.2126, 0.7152, 0.0722
            const float luma = (0.2126f * m[0] + 0.7152f * m[1] + 0.0722f * m[2]);
            const float alpha = m[3];
            
            float weight = 1.0f;
            switch (type) {
                case TrackMatteType::Alpha:          weight = alpha; break;
                case TrackMatteType::AlphaInverted:  weight = 1.0f - alpha; break;
                case TrackMatteType::Luma:           weight = luma * alpha; break;
                case TrackMatteType::LumaInverted:   weight = 1.0f - (luma * alpha); break;
                default: break;
            }
            
            s[0] *= weight;
            s[1] *= weight;
            s[2] *= weight;
            s[3] *= weight;
        }
    }
}

void composite_surface(
    renderer2d::SurfaceRGBA& dst,
    const renderer2d::SurfaceRGBA& src,
    int offset_x,
    int offset_y,
    renderer2d::BlendMode blend_mode) {

    for (std::uint32_t y = 0; y < src.height(); ++y) {
        for (std::uint32_t x = 0; x < src.width(); ++x) {
            const int dx = offset_x + static_cast<int>(x);
            const int dy = offset_y + static_cast<int>(y);
            if (dx < 0 || dy < 0) continue;
            
            const std::uint32_t ux = static_cast<std::uint32_t>(dx);
            const std::uint32_t uy = static_cast<std::uint32_t>(dy);
            if (ux >= dst.width() || uy >= dst.height()) continue;

            const renderer2d::Color pixel = src.get_pixel(x, y);
            if (pixel.a == 0) continue;

            if (blend_mode == renderer2d::BlendMode::Normal) {
                dst.blend_pixel(ux, uy, pixel);
            } else {
                const auto dest = dst.try_get_pixel(ux, uy);
                if (dest) {
                    const renderer2d::Color blended = renderer2d::blend_mode_color(pixel, *dest, blend_mode);
                    dst.set_pixel(ux, uy, renderer2d::detail::composite_src_over_linear(blended, *dest));
                }
            }
        }
    }
}

} // namespace tachyon
