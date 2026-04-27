#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"

#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/text/animation/text_animator_utils.h"
#include "tachyon/text/layout/layout.h"
#include "tachyon/text/rendering/text_raster_surface.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/layer_renderer_procedural.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/layer_renderer_interface.h"


#include <algorithm>
#include <cmath>
#include <limits>
#include <span>
#include <string>

namespace tachyon::renderer2d {
namespace {

renderer2d::Color to_color(const ::tachyon::ColorSpec& spec) {
    return {
        static_cast<float>(spec.r) / 255.0f,
        static_cast<float>(spec.g) / 255.0f,
        static_cast<float>(spec.b) / 255.0f,
        static_cast<float>(spec.a) / 255.0f
    };
}

std::shared_ptr<SurfaceRGBA> make_canvas(
    const scene::EvaluatedCompositionState& state,
    const std::optional<RectI>& target_rect,
    RenderContext2D& context) {

    const std::uint32_t width = target_rect.has_value()
        ? static_cast<std::uint32_t>(std::max(1, target_rect->width))
        : static_cast<std::uint32_t>(std::max(1, state.width));
    const std::uint32_t height = target_rect.has_value()
        ? static_cast<std::uint32_t>(std::max(1, target_rect->height))
        : static_cast<std::uint32_t>(std::max(1, state.height));

    auto surface = std::make_shared<SurfaceRGBA>(width, height);
    if (surface) {
        surface->set_profile(context.cms.working_profile);
        surface->clear(Color::transparent());
    }
    return surface;
}

void fill_rect(::tachyon::text::TextRasterSurface& surface, int x, int y, int w, int h, renderer2d::Color color) {
    if (w <= 0 || h <= 0) {
        return;
    }
    for (int py = 0; py < h; ++py) {
        for (int px = 0; px < w; ++px) {
            const int sx = x + px;
            const int sy = y + py;
            if (sx < 0 || sy < 0) {
                continue;
            }
            surface.blend_pixel(static_cast<std::uint32_t>(sx), static_cast<std::uint32_t>(sy), color, 255);
        }
    }
}

void blit_text_surface(
    SurfaceRGBA& dst,
    const ::tachyon::text::TextRasterSurface& src,
    int offset_x,
    int offset_y) {

    const auto& pixels = src.rgba_pixels();
    const std::uint32_t src_w = src.width();
    const std::uint32_t src_h = src.height();

    for (std::uint32_t y = 0; y < src_h; ++y) {
        for (std::uint32_t x = 0; x < src_w; ++x) {
            const std::size_t idx = (static_cast<std::size_t>(y) * src_w + x) * 4U;
            const std::uint8_t a = pixels[idx + 3U];
            if (a == 0U) {
                continue;
            }

            const int dx = offset_x + static_cast<int>(x);
            const int dy = offset_y + static_cast<int>(y);
            if (dx < 0 || dy < 0) {
                continue;
            }

            dst.blend_pixel(
                static_cast<std::uint32_t>(dx),
                static_cast<std::uint32_t>(dy),
                Color{
                    static_cast<float>(pixels[idx + 0U]) / 255.0f,
                    static_cast<float>(pixels[idx + 1U]) / 255.0f,
                    static_cast<float>(pixels[idx + 2U]) / 255.0f,
                    static_cast<float>(a) / 255.0f
                }
            );
        }
    }
}

std::string make_precomp_cache_key(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    const FrameRenderTask& task) {

    std::string key;
    key.reserve(128);
    key += "precomp:";
    key += state.composition_id;
    key += ':';
    key += layer.id.empty() ? layer.layer_id : layer.id;
    key += ':';
    key += layer.precomp_id.value_or("");
    key += ":f";
    key += std::to_string(task.frame_number);
    key += ":k";
    key += task.cache_key.value;
    return key;
}

// -------------------------------------------------------------------------------------------------
// Decoupled Renderers
// -------------------------------------------------------------------------------------------------

class TextLayerRenderer : public ILayerRenderer {
public:
    bool render(
        const scene::EvaluatedLayerState& layer,
        const scene::EvaluatedCompositionState& state,
        RenderContext2D& context,
        const std::optional<RectI>& target_rect,
        std::shared_ptr<SurfaceRGBA>& surface) const override {

        const int origin_x = target_rect.has_value() ? target_rect->x : 0;
        const int origin_y = target_rect.has_value() ? target_rect->y : 0;
        const int base_x = static_cast<int>(std::lround(layer.local_transform.position.x)) - origin_x;
        const int base_y = static_cast<int>(std::lround(layer.local_transform.position.y)) - origin_y;
        const float scale_x = std::max(0.001f, layer.local_transform.scale.x);
        const float scale_y = std::max(0.001f, layer.local_transform.scale.y);

        const auto* registry = context.font_registry;
        if (registry == nullptr) return false;

        const ::tachyon::text::Font* font = nullptr;
        if (!layer.font_id.empty()) font = registry->find(layer.font_id);
        if (font == nullptr) font = registry->default_font();
        if (font == nullptr) return false;

        ::tachyon::text::TextStyle style;
        style.pixel_size = static_cast<std::uint32_t>(std::max(1.0f, layer.font_size));
        style.fill_color = to_color(layer.fill_color);
        style.gradient.reset();

        ::tachyon::text::TextBox box;
        box.width = static_cast<std::uint32_t>(std::max(1, layer.width > 0 ? layer.width : state.width));
        box.height = static_cast<std::uint32_t>(std::max(1, layer.height > 0 ? layer.height : state.height));
        box.multiline = true;

        const ::tachyon::text::TextAlignment alignment =
            layer.text_alignment == 1 ? ::tachyon::text::TextAlignment::Center :
            layer.text_alignment == 2 ? ::tachyon::text::TextAlignment::Right :
            ::tachyon::text::TextAlignment::Left;

        ::tachyon::text::TextLayoutOptions layout_options;
        const auto layout = ::tachyon::text::layout_text(*font, layer.text_content, style, box, alignment, layout_options);

        if (!layout.glyphs.empty()) {
            const float time_seconds = static_cast<float>(layer.local_time_seconds);
            ::tachyon::text::TextAnimationOptions animation{};
            animation.time_seconds = time_seconds;
            animation.animators = std::span<const ::tachyon::TextAnimatorSpec>(layer.text_animators.data(), layer.text_animators.size());
            const auto paints = ::tachyon::text::resolve_glyph_paints(*font, layout, animation);

            ::tachyon::text::TextRasterSurface text_surface(surface->width(), surface->height());

            for (const auto& highlight : layer.text_highlights) {
                if (highlight.end_glyph <= highlight.start_glyph || highlight.start_glyph >= layout.glyphs.size()) continue;
                const std::size_t end = std::min(highlight.end_glyph, layout.glyphs.size());
                int min_x = std::numeric_limits<int>::max(), min_y = std::numeric_limits<int>::max();
                int max_x = std::numeric_limits<int>::min(), max_y = std::numeric_limits<int>::min();
                
                for (std::size_t i = highlight.start_glyph; i < end; ++i) {
                    const auto& g = layout.glyphs[i];
                    min_x = std::min(min_x, g.x); min_y = std::min(min_y, g.y);
                    max_x = std::max(max_x, g.x + g.width); max_y = std::max(max_y, g.y + g.height);
                }
                if (min_x < max_x && min_y < max_y) {
                    fill_rect(text_surface,
                        base_x + static_cast<int>(std::lround(min_x * scale_x)) - highlight.padding_x,
                        base_y + static_cast<int>(std::lround(min_y * scale_y)) - highlight.padding_y,
                        static_cast<int>(std::lround((max_x - min_x) * scale_x)) + highlight.padding_x * 2,
                        static_cast<int>(std::lround((max_y - min_y) * scale_y)) + highlight.padding_y * 2,
                        to_color(highlight.color));
                }
            }

            for (const auto& paint : paints) {
                if (paint.glyph == nullptr) continue;
                const int dx = base_x + static_cast<int>(std::lround(static_cast<float>(paint.base_x) * scale_x));
                const int dy = base_y + static_cast<int>(std::lround(static_cast<float>(paint.base_y) * scale_y));
                const int dw = std::max(1, static_cast<int>(std::lround(static_cast<float>(paint.target_width) * scale_x)));
                const int dh = std::max(1, static_cast<int>(std::lround(static_cast<float>(paint.target_height) * scale_y)));
                renderer2d::Color final_color = to_color(paint.fill_color);
                final_color.a *= paint.opacity;
                text_surface.render_glyph(*paint.glyph, dx, dy, dw, dh, final_color);
            }

            blit_text_surface(*surface, text_surface, 0, 0);
        }
        return true;
    }
};

class SolidLayerRenderer : public ILayerRenderer {
public:
    bool render(
        const scene::EvaluatedLayerState& layer,
        const scene::EvaluatedCompositionState& state,
        RenderContext2D& context,
        const std::optional<RectI>& target_rect,
        std::shared_ptr<SurfaceRGBA>& surface) const override {
        
        (void)state; (void)context;
        
        const int origin_x = target_rect.has_value() ? target_rect->x : 0;
        const int origin_y = target_rect.has_value() ? target_rect->y : 0;
        const int base_x = static_cast<int>(std::lround(layer.local_transform.position.x)) - origin_x;
        const int base_y = static_cast<int>(std::lround(layer.local_transform.position.y)) - origin_y;
        const float scale_x = std::max(0.001f, layer.local_transform.scale.x);
        const float scale_y = std::max(0.001f, layer.local_transform.scale.y);

        const int width = std::max(1, static_cast<int>(std::lround(static_cast<float>(layer.width) * scale_x)));
        const int height = std::max(1, static_cast<int>(std::lround(static_cast<float>(layer.height) * scale_y)));
        surface->fill_rect(RectI{base_x, base_y, width, height}, to_color(layer.fill_color), true);
        return true;
    }
};

class ProceduralLayerRenderer : public ILayerRenderer {
public:
    bool render(
        const scene::EvaluatedLayerState& layer,
        const scene::EvaluatedCompositionState& state,
        RenderContext2D& context,
        const std::optional<RectI>& target_rect,
        std::shared_ptr<SurfaceRGBA>& surface) const override {
        
        (void)state; (void)context;
        render_procedural_layer(*surface, layer, layer.local_time_seconds, target_rect);
        return true;
    }
};

// Auto-register built-in layer renderers
struct BuiltinRendererRegistrar {
    BuiltinRendererRegistrar() {
        auto& reg = LayerRendererRegistry::get();
        reg.register_renderer(scene::LayerType::Solid, std::make_unique<SolidLayerRenderer>());
        reg.register_renderer(scene::LayerType::Text, std::make_unique<TextLayerRenderer>());
        reg.register_renderer(scene::LayerType::Procedural, std::make_unique<ProceduralLayerRenderer>());
    }
};
static BuiltinRendererRegistrar s_builtin_registrar;

} // namespace

std::shared_ptr<SurfaceRGBA> render_precomp_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext2D& context) {

    if (context.precomp_cache && layer.nested_composition) {
        const std::string cache_key = make_precomp_cache_key(layer, state, task);
        if (auto cached = context.precomp_cache->get(cache_key)) {
            return std::const_pointer_cast<SurfaceRGBA>(cached);
        }

        auto nested = render_evaluated_composition_2d(*layer.nested_composition, plan, task, context);
        if (nested.surface) {
            auto surface_copy = std::make_shared<SurfaceRGBA>(*nested.surface);
            context.precomp_cache->put(cache_key, surface_copy);
            return surface_copy;
        }
        return nested.surface;
    }

    if (!state.layers.empty() && layer.nested_composition) {
        auto nested = render_evaluated_composition_2d(*layer.nested_composition, plan, task, context);
        return nested.surface;
    }
    return make_canvas(state, std::nullopt, context);
}

std::shared_ptr<SurfaceRGBA> render_simple_layer_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    RenderContext2D& context,
    const std::optional<RectI>& target_rect) {

    auto surface = make_canvas(state, target_rect, context);
    if (!surface) {
        return surface;
    }

    auto* renderer = LayerRendererRegistry::get().get_renderer(layer.type);
    if (renderer) {
        renderer->render(layer, state, context, target_rect, surface);
    } else {
        // Fallback / Log unsupported type if needed
    }

    return surface;
}

} // namespace tachyon::renderer2d
