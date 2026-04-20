#include "tachyon/renderer2d/evaluated_composition_renderer.h"

#include "tachyon/renderer2d/effect_host.h"
#include "tachyon/renderer2d/framebuffer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>

namespace tachyon {
namespace {

renderer2d::Color from_color_spec(const ColorSpec& spec) {
    return renderer2d::Color{
        spec.r,
        spec.g,
        spec.b,
        spec.a
    };
}

renderer2d::Color apply_opacity(renderer2d::Color color, double opacity) {
    const float clamped = std::clamp(static_cast<float>(opacity), 0.0f, 1.0f);
    color.a = static_cast<std::uint8_t>(std::lround(static_cast<float>(color.a) * clamped));
    return color;
}

renderer2d::RectI layer_rect(const scene::EvaluatedLayerState& layer, std::int64_t comp_width, std::int64_t comp_height) {
    const std::int64_t base_width = layer.width > 0 ? layer.width : comp_width;
    const std::int64_t base_height = layer.height > 0 ? layer.height : comp_height;
    const float scale_x = std::max(0.0f, std::abs(layer.local_transform.scale.x));
    const float scale_y = std::max(0.0f, std::abs(layer.local_transform.scale.y));
    const int width = std::max(1, static_cast<int>(std::lround(static_cast<double>(base_width) * static_cast<double>(scale_x))));
    const int height = std::max(1, static_cast<int>(std::lround(static_cast<double>(base_height) * static_cast<double>(scale_y))));
    return renderer2d::RectI{
        static_cast<int>(std::lround(layer.local_transform.position.x)),
        static_cast<int>(std::lround(layer.local_transform.position.y)),
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

renderer2d::SurfaceRGBA make_surface(std::int64_t width, std::int64_t height) {
    const std::uint32_t w = static_cast<std::uint32_t>(std::max<std::int64_t>(1, width));
    const std::uint32_t h = static_cast<std::uint32_t>(std::max<std::int64_t>(1, height));
    renderer2d::SurfaceRGBA surface(w, h);
    surface.clear(renderer2d::Color::transparent());
    return surface;
}

void multiply_surface_alpha(renderer2d::SurfaceRGBA& surface, float factor) {
    const float clamped = std::clamp(factor, 0.0f, 1.0f);
    if (clamped >= 0.9999f) {
        return;
    }

    for (std::uint32_t y = 0; y < surface.height(); ++y) {
        for (std::uint32_t x = 0; x < surface.width(); ++x) {
            const renderer2d::Color px = surface.get_pixel(x, y);
            if (px.a == 0) {
                continue;
            }
            const renderer2d::Color scaled{
                static_cast<std::uint8_t>(std::lround(static_cast<float>(px.r) * clamped)),
                static_cast<std::uint8_t>(std::lround(static_cast<float>(px.g) * clamped)),
                static_cast<std::uint8_t>(std::lround(static_cast<float>(px.b) * clamped)),
                static_cast<std::uint8_t>(std::lround(static_cast<float>(px.a) * clamped))
            };
            surface.set_pixel(x, y, scaled);
        }
    }
}

void apply_mask(renderer2d::SurfaceRGBA& surface, const renderer2d::SurfaceRGBA& mask) {
    const std::uint32_t width = std::min(surface.width(), mask.width());
    const std::uint32_t height = std::min(surface.height(), mask.height());
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const renderer2d::Color src = surface.get_pixel(x, y);
            const renderer2d::Color m = mask.get_pixel(x, y);
            if (src.a == 0 || m.a == 0) {
                surface.set_pixel(x, y, renderer2d::Color::transparent());
                continue;
            }
            const float factor = static_cast<float>(m.a) / 255.0f;
            surface.set_pixel(x, y, renderer2d::Color{
                static_cast<std::uint8_t>(std::lround(static_cast<float>(src.r) * factor)),
                static_cast<std::uint8_t>(std::lround(static_cast<float>(src.g) * factor)),
                static_cast<std::uint8_t>(std::lround(static_cast<float>(src.b) * factor)),
                static_cast<std::uint8_t>(std::lround(static_cast<float>(src.a) * factor))
            });
        }
    }
}

void composite_surface(
    renderer2d::SurfaceRGBA& dst,
    const renderer2d::SurfaceRGBA& src,
    int offset_x,
    int offset_y,
    renderer2d::BlendMode blend_mode = renderer2d::BlendMode::Normal) {

    for (std::uint32_t y = 0; y < src.height(); ++y) {
        for (std::uint32_t x = 0; x < src.width(); ++x) {
            const int dx = offset_x + static_cast<int>(x);
            const int dy = offset_y + static_cast<int>(y);
            if (dx < 0 || dy < 0) {
                continue;
            }
            const std::uint32_t ux = static_cast<std::uint32_t>(dx);
            const std::uint32_t uy = static_cast<std::uint32_t>(dy);
            if (ux >= dst.width() || uy >= dst.height()) {
                continue;
            }

            const renderer2d::Color pixel = src.get_pixel(x, y);
            if (pixel.a == 0) {
                continue;
            }

            if (blend_mode == renderer2d::BlendMode::Normal) {
                dst.blend_pixel(ux, uy, pixel);
                continue;
            }

            const auto dest = dst.try_get_pixel(ux, uy);
            if (!dest.has_value()) {
                continue;
            }
            dst.set_pixel(ux, uy, renderer2d::blend_mode_color(pixel, *dest, blend_mode));
        }
    }
}

renderer2d::EffectHost& builtin_effect_host() {
    static std::unique_ptr<renderer2d::EffectHost> host = [] {
        auto created = renderer2d::create_effect_host();
        renderer2d::EffectHost::register_builtins(*created);
        return created;
    }();
    return *host;
}

renderer2d::EffectHost& effect_host_for(renderer2d::RenderContext& context) {
    if (context.effects) {
        return *context.effects;
    }
    return builtin_effect_host();
}

renderer2d::EffectParams effect_params_from_spec(const EffectSpec& spec) {
    renderer2d::EffectParams params;
    for (const auto& [key, value] : spec.scalars) {
        params.scalars.emplace(key, static_cast<float>(value));
    }
    for (const auto& [key, value] : spec.colors) {
        params.colors.emplace(key, renderer2d::Color{value.r, value.g, value.b, value.a});
    }
    for (const auto& [key, value] : spec.strings) {
        params.strings.emplace(key, value);
    }
    return params;
}

renderer2d::SurfaceRGBA apply_effect_pipeline(
    const renderer2d::SurfaceRGBA& input,
    const std::vector<EffectSpec>& effects,
    renderer2d::EffectHost& host) {

    renderer2d::SurfaceRGBA current = input;
    for (const auto& effect : effects) {
        if (!effect.enabled || effect.type.empty()) {
            continue;
        }
        current = host.apply(effect.type, current, effect_params_from_spec(effect));
    }
    return current;
}

renderer2d::SurfaceRGBA render_layer_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    renderer2d::RenderContext& context);

renderer2d::SurfaceRGBA render_precomp_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    renderer2d::RenderContext& context) {

    if (!layer.nested_composition) {
        return make_surface(state.width, state.height);
    }

    const std::string cache_key = layer.precomp_id.value_or(layer.id) + ":" + std::to_string(task.frame_number);
    if (context.precomp_cache) {
        if (const auto cached = context.precomp_cache->lookup(cache_key)) {
            return *cached;
        }
    }

    const RasterizedFrame2D nested = render_evaluated_composition_2d(*layer.nested_composition, plan, task, context);
    renderer2d::SurfaceRGBA surface = nested.surface.has_value()
        ? *nested.surface
        : make_surface(layer.nested_composition->width, layer.nested_composition->height);

    if (context.precomp_cache) {
        context.precomp_cache->store(cache_key, std::make_shared<renderer2d::SurfaceRGBA>(surface));
    }
    return surface;
}

renderer2d::SurfaceRGBA render_simple_layer_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state) {

    renderer2d::SurfaceRGBA surface = make_surface(state.width, state.height);

    if (!layer.visible || !layer.enabled || !layer.active) {
        return surface;
    }

    const renderer2d::RectI rect = layer.shape_path.has_value()
        ? shape_bounds_from_path(*layer.shape_path)
        : layer_rect(layer, state.width, state.height);

    if (rect.width <= 0 || rect.height <= 0) {
        return surface;
    }

    renderer2d::Color color = from_color_spec(layer.fill_color);
    color = apply_opacity(color, layer.opacity);
    surface.fill_rect(rect, color, true);
    return surface;
}

renderer2d::SurfaceRGBA render_layer_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    renderer2d::RenderContext& context) {

    switch (layer.type) {
    case scene::LayerType::Precomp:
        return render_precomp_surface(layer, state, plan, task, context);
    case scene::LayerType::Mask:
        return render_simple_layer_surface(layer, state);
    case scene::LayerType::Solid:
    case scene::LayerType::Shape:
    case scene::LayerType::Image:
    case scene::LayerType::Text:
    default:
        return render_simple_layer_surface(layer, state);
    }
}

} // namespace

RasterizedFrame2D render_evaluated_composition_2d(
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    renderer2d::RenderContext& context) {

    RasterizedFrame2D frame;
    frame.frame_number = task.frame_number;
    frame.width = state.width;
    frame.height = state.height;
    frame.layer_count = state.layers.size();
    frame.backend_name = "cpu-evaluated-composition";
    frame.cache_key = task.cache_key.value;
    frame.note = "Frame rasterized from evaluated composition";
    frame.surface.emplace(static_cast<std::uint32_t>(std::max<std::int64_t>(1, state.width)),
                          static_cast<std::uint32_t>(std::max<std::int64_t>(1, state.height)));
    if (!frame.surface.has_value()) {
        return frame;
    }

    auto& dst = *frame.surface;
    dst.clear(renderer2d::Color::transparent());

    renderer2d::EffectHost& host = effect_host_for(context);
    std::optional<renderer2d::SurfaceRGBA> active_mask;

    for (const auto& layer : state.layers) {
        if (!layer.enabled || !layer.active) {
            continue;
        }

        if (layer.type == scene::LayerType::Mask) {
            active_mask = render_layer_surface(layer, state, plan, task, context);
            continue;
        }

        renderer2d::SurfaceRGBA layer_surface = render_layer_surface(layer, state, plan, task, context);

        if (layer.is_adjustment_layer) {
            layer_surface = apply_effect_pipeline(layer_surface, layer.effects, host);
            if (active_mask.has_value()) {
                apply_mask(layer_surface, *active_mask);
            }
            composite_surface(dst, layer_surface, 0, 0, renderer2d::BlendMode::Normal);
            continue;
        }

        if (layer.track_matte_layer_index.has_value()) {
            const std::size_t matte_index = *layer.track_matte_layer_index;
            if (matte_index < state.layers.size()) {
                renderer2d::SurfaceRGBA matte_surface = render_layer_surface(state.layers[matte_index], state, plan, task, context);
                apply_mask(layer_surface, matte_surface);
            }
        }

        if (active_mask.has_value()) {
            apply_mask(layer_surface, *active_mask);
        }

        composite_surface(dst, layer_surface, 0, 0, static_cast<renderer2d::BlendMode>(
            layer.blend_mode == "additive" ? renderer2d::BlendMode::Additive :
            layer.blend_mode == "multiply" ? renderer2d::BlendMode::Multiply :
            layer.blend_mode == "screen" ? renderer2d::BlendMode::Screen :
            layer.blend_mode == "overlay" ? renderer2d::BlendMode::Overlay :
            layer.blend_mode == "soft_light" || layer.blend_mode == "softLight" ? renderer2d::BlendMode::SoftLight :
            renderer2d::BlendMode::Normal));
    }

    return frame;
}

} // namespace tachyon
