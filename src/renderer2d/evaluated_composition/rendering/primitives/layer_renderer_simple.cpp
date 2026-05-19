#include "tachyon/renderer2d/resource/precomp_cache.h"
#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"

#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/pipeline/pipeline_helpers.h"
#include "layer_renderer_simple.h"

#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/rendering/primitives/layer_renderer_procedural.h"
#include <iostream>
#include "tachyon/renderer2d/evaluated_composition/rendering/core/layer_renderer_interface.h"
#include "tachyon/core/media/media_provider.h"
#include "tachyon/core/media/asset_resolver_interface.h"
#include "tachyon/render/intent_builder.h"
#include "tachyon/renderer2d/resource/resource_provider.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/runtime/execution/session/render_internal.h"
#include "tachyon/core/scene/transform_resolver.h"
#include "tachyon/renderer2d/evaluated_composition/utilities/composition_utils.h"


#include <algorithm>
#include <iostream>
#include <cmath>
#include <limits>
#include <cstdlib>
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
    RenderContext& context) {

    const std::uint32_t width = target_rect.has_value()
        ? static_cast<std::uint32_t>(std::max(1, target_rect->width))
        : static_cast<std::uint32_t>(std::max(1u, state.width));
    const std::uint32_t height = target_rect.has_value()
        ? static_cast<std::uint32_t>(std::max(1, target_rect->height))
        : static_cast<std::uint32_t>(std::max(1u, state.height));

    auto surface = context.surface_pool 
        ? context.surface_pool->acquire(width, height)
        : std::make_shared<SurfaceRGBA>(width, height);

    if (surface) {
        surface->set_profile(context.cms.working_profile);
        surface->clear(Color::transparent());
    }
    return surface;
}

void fill_rect_direct_worker(SurfaceRGBA& surface, int x, int y, int w, int h, renderer2d::Color color) {
    if (w <= 0 || h <= 0) return;
    surface.fill_rect(RectI{x, y, w, h}, color, true);
}

#ifdef TACHYON_ENABLE_TEXT
void render_glyph_direct(
    SurfaceRGBA& surface,
    const ::tachyon::text::GlyphBitmap& bitmap,
    int dx, int dy, int dw, int dh,
    renderer2d::Color color) {
    
    const std::uint32_t sw = bitmap.width;
    const std::uint32_t sh = bitmap.height;
    if (sw == 0 || sh == 0) return;

    if (dw == static_cast<int>(sw) && dh == static_cast<int>(sh)) {
        // Fast path for 1:1 rendering
        for (int y = 0; y < dh; ++y) {
            if (dy + y < 0 || dy + y >= static_cast<int>(surface.height())) continue;
            const std::uint8_t* src_row = &bitmap.alpha_mask[y * sw];
            for (int x = 0; x < dw; ++x) {
                if (dx + x < 0 || dx + x >= static_cast<int>(surface.width())) continue;
                const std::uint8_t alpha = src_row[x];
                if (alpha == 0) continue;
                surface.blend_pixel(static_cast<std::uint32_t>(dx + x), static_cast<std::uint32_t>(dy + y), color, alpha);
            }
        }
        return;
    }

    // Bilinear path for smooth scaling
    const float x_ratio = static_cast<float>(sw) / static_cast<float>(dw);
    const float y_ratio = static_cast<float>(sh) / static_cast<float>(dh);

    for (int y = 0; y < dh; ++y) {
        const int target_y = dy + y;
        if (target_y < 0 || target_y >= static_cast<int>(surface.height())) continue;

        const float sy = (static_cast<float>(y) + 0.5f) * y_ratio - 0.5f;
        const int y0 = std::clamp(static_cast<int>(std::floor(sy)), 0, static_cast<int>(sh) - 1);
        const int y1 = std::clamp(y0 + 1, 0, static_cast<int>(sh) - 1);
        const float fy = sy - std::floor(sy);

        for (int x = 0; x < dw; ++x) {
            const int target_x = dx + x;
            if (target_x < 0 || target_x >= static_cast<int>(surface.width())) continue;

            const float sx = (static_cast<float>(x) + 0.5f) * x_ratio - 0.5f;
            const int x0 = std::clamp(static_cast<int>(std::floor(sx)), 0, static_cast<int>(sw) - 1);
            const int x1 = std::clamp(x0 + 1, 0, static_cast<int>(sw) - 1);
            const float fx = sx - std::floor(sx);

            const float a00 = static_cast<float>(bitmap.alpha_mask[y0 * sw + x0]);
            const float a10 = static_cast<float>(bitmap.alpha_mask[y0 * sw + x1]);
            const float a01 = static_cast<float>(bitmap.alpha_mask[y1 * sw + x0]);
            const float a11 = static_cast<float>(bitmap.alpha_mask[y1 * sw + x1]);

            const float alpha_f = a00 * (1.0f - fx) * (1.0f - fy) +
                                  a10 * fx * (1.0f - fy) +
                                  a01 * (1.0f - fx) * fy +
                                  a11 * fx * fy;

            const std::uint8_t alpha = static_cast<std::uint8_t>(std::clamp(alpha_f, 0.0f, 255.0f));
            if (alpha == 0) continue;
            
            surface.blend_pixel(static_cast<std::uint32_t>(target_x), static_cast<std::uint32_t>(target_y), color, alpha);
        }
    }
}
#endif

std::string make_precomp_cache_key(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    const FrameRenderTask& task) {

    std::string key;
    key.reserve(128);
    key += "precomp:";
    key += state.composition_id;
    key += ':';
    key += layer.identity.id;
    key += ':';
    key += layer.source.precomp_id().value_or("");
    key += ":f";
    key += std::to_string(task.frame_number);
    key += ":k";
    key += task.cache_key.value;
    return key;
}

// -------------------------------------------------------------------------------------------------
// Decoupled Renderers
// -------------------------------------------------------------------------------------------------

#ifdef TACHYON_ENABLE_TEXT
class TextLayerRenderer : public ILayerRenderer {
public:
    bool render(
        const scene::EvaluatedLayerState& layer,
        const scene::EvaluatedCompositionState& state,
        const render::RenderIntent& intent,
        RenderContext& context,
        const std::optional<RectI>& target_rect,
        std::shared_ptr<SurfaceRGBA>& surface) const override {
        (void)intent;
        (void)state;

        const int origin_x = target_rect.has_value() ? target_rect->x : 0;
        const int origin_y = target_rect.has_value() ? target_rect->y : 0;

        const auto* registry = context.font_registry;
        if (registry == nullptr) return false;

        const ::tachyon::text::Font* font = nullptr;
        if (!layer.text.font_id.empty()) font = registry->find(layer.text.font_id);
        
        if (font == nullptr) {
            font = registry->default_font();
        }
        if (font == nullptr) return false;

        ::tachyon::text::TextStyle style;
        style.pixel_size = static_cast<std::uint32_t>(std::max(1.0f, layer.text.font_size));
        style.fill_color = to_color(layer.text.fill_color);

        ::tachyon::text::TextLayoutOptions layout_options;
        layout_options.fixed_pitch = ::tachyon::text::prefers_fixed_pitch_layout(layer.text.animators);
        
        const auto layout = ::tachyon::text::layout_text(*font, layer.text.content, style, layer.text.box, layout_options);
        
        const math::Vector2 bounds = {static_cast<float>(layout.box_width), static_cast<float>(layout.box_height)};
        const auto resolved = resolve_transform_2d(layer.transform.local_transform, bounds);

        if (layout.glyphs.empty()) {
            return true;
        }

        const bool has_animations = !layer.text.animators.empty();
        const bool has_highlights = !layer.text.highlights.empty();

        if (!has_animations && !has_highlights) {
            for (const auto& glyph : layout.glyphs) {
                const auto* bitmap = glyph.resolved_glyph;
                if (bitmap == nullptr) continue;

                math::Vector2 wp_base = resolved.apply(math::Vector2{static_cast<float>(glyph.position.x), static_cast<float>(glyph.position.y)});
                const int dx = static_cast<int>(std::lround(wp_base.x)) - origin_x;
                const int dy = static_cast<int>(std::lround(wp_base.y)) - origin_y;
                const int dw = std::max(1, static_cast<int>(std::lround(glyph.width * resolved.scale.x)));
                const int dh = std::max(1, static_cast<int>(std::lround(glyph.height * resolved.scale.y)));
                
                render_glyph_direct(*surface, *bitmap, dx, dy, dw, dh, to_color(layer.text.fill_color));
            }
            return true;
        }

        const float time_seconds = static_cast<float>(layer.playback.local_time_seconds);
        ::tachyon::text::TextAnimationOptions animation_opts{};
        animation_opts.time_seconds = time_seconds;
        animation_opts.animators = layer.text.animators;
        
        const auto paints = ::tachyon::text::resolve_glyph_paints(*font, layout, animation_opts);

        for (const auto& highlight : layer.text.highlights) {
            if (highlight.end_glyph <= highlight.start_glyph || highlight.start_glyph >= layout.glyphs.size()) continue;
            std::size_t h_end_idx = highlight.end_glyph;
            if (h_end_idx > layout.glyphs.size()) h_end_idx = layout.glyphs.size();
            float h_min_x = 1e30f, h_min_y = 1e30f;
            float h_max_x = -1e30f, h_max_y = -1e30f;
            
            for (std::size_t i = highlight.start_glyph; i < h_end_idx; ++i) {
                const auto& g = layout.glyphs[i];
                h_min_x = (std::min)(h_min_x, g.position.x); h_min_y = (std::min)(h_min_y, g.position.y);
                h_max_x = (std::max)(h_max_x, g.position.x + g.width); h_max_y = (std::max)(h_max_y, g.position.y + g.height);
            }
            if (h_min_x < h_max_x && h_min_y < h_max_y) {
                math::Vector2 h_wp0 = resolved.apply(math::Vector2{h_min_x, h_min_y});
                math::Vector2 h_wp1 = resolved.apply(math::Vector2{h_max_x, h_max_y});
                
                int h_final_x = (int)(h_wp0.x + 0.5f) - origin_x - (int)highlight.padding_x;
                int h_final_y = (int)(h_wp0.y + 0.5f) - origin_y - (int)highlight.padding_y;
                int h_final_w = (int)(h_wp1.x - h_wp0.x + 0.5f) + (int)highlight.padding_x * 2;
                int h_final_h = (int)(h_wp1.y - h_wp0.y + 0.5f) + (int)highlight.padding_y * 2;
                renderer2d::Color h_final_color = to_color(highlight.color);
                
                fill_rect_direct_worker(*surface, h_final_x, h_final_y, h_final_w, h_final_h, h_final_color);
            }
        }

        for (const auto& paint : paints) {
            const auto* bitmap = paint.glyph;
            if (bitmap == nullptr) continue;

            math::Vector2 wp_base = resolved.apply(math::Vector2{static_cast<float>(paint.base_x), static_cast<float>(paint.base_y)});
            const int dx = static_cast<int>(std::lround(wp_base.x)) - origin_x;
            const int dy = static_cast<int>(std::lround(wp_base.y)) - origin_y;
            const int dw = std::max(1, static_cast<int>(std::lround(paint.target_width * resolved.scale.x)));
            const int dh = std::max(1, static_cast<int>(std::lround(paint.target_height * resolved.scale.y)));
            
            renderer2d::Color final_color = to_color(paint.fill_color);
            final_color.a *= paint.opacity;
            
            if (paint.is_cursor) {
                fill_rect_direct_worker(*surface, dx, dy, dw, dh, final_color);
            } else {
                render_glyph_direct(*surface, *bitmap, dx, dy, dw, dh, final_color);
            }
        }
        return true;
    }
};
#endif

#ifdef TACHYON_ENABLE_MEDIA
class ImageLayerRenderer : public ILayerRenderer {
public:
    bool render(
        const scene::EvaluatedLayerState& layer,
        const scene::EvaluatedCompositionState& state,
        const render::RenderIntent& intent,
        RenderContext& context,
        const std::optional<RectI>& target_rect,
        std::shared_ptr<SurfaceRGBA>& surface) const override {
        (void)intent;
        (void)state;
        (void)target_rect;

        if (context.asset_resolver == nullptr) {
            return false;
        }

        const auto media_source = resolve_media_source(layer, context);
        if (!media_source.has_value()) {
            return false;
        }

        auto image = context.asset_resolver->resolve_image_shared(media_source->string());
        if (image == nullptr) {
            return false;
        }

        surface = std::const_pointer_cast<SurfaceRGBA>(std::static_pointer_cast<const SurfaceRGBA>(image));
        if (surface) {
            surface = std::make_shared<SurfaceRGBA>(*image);
        }
        return true;
    }
};

class VideoLayerRenderer : public ILayerRenderer {
public:
    bool render(
        const scene::EvaluatedLayerState& layer,
        const scene::EvaluatedCompositionState& state,
        const render::RenderIntent& intent,
        RenderContext& context,
        const std::optional<RectI>& target_rect,
        std::shared_ptr<SurfaceRGBA>& surface) const override {
        (void)intent;
        (void)state;
        (void)target_rect;

        if (context.media == nullptr) {
            return false;
        }

        const auto media_source = resolve_media_source(layer, context);
        if (!media_source.has_value()) {
            return false;
        }

        const auto* frame = context.media->get_video_frame(*media_source, layer.playback.local_time_seconds);
        if (frame == nullptr) {
            surface->clear({0, 0, 1, 1}); // BLUE
            return true;
        }

        if (frame) {
            *surface = *frame;
        }
        return true;
    }
};
#endif

class SolidLayerRenderer : public ILayerRenderer {
public:
    bool render(
        const scene::EvaluatedLayerState& layer,
        const scene::EvaluatedCompositionState& state,
        const render::RenderIntent& intent,
        RenderContext& context,
        const std::optional<RectI>& target_rect,
        std::shared_ptr<SurfaceRGBA>& surface) const override {
        (void)intent;
        (void)state; (void)context;
        
        const int origin_x = target_rect.has_value() ? target_rect->x : 0;
        const int origin_y = target_rect.has_value() ? target_rect->y : 0;
        const float scale_x = std::max(0.001f, layer.transform.local_transform.scale.x);
        const float scale_y = std::max(0.001f, layer.transform.local_transform.scale.y);
        const int base_x = static_cast<int>(std::lround(layer.transform.local_transform.position.x - layer.transform.local_transform.anchor_point.x * scale_x)) - origin_x;
        const int base_y = static_cast<int>(std::lround(layer.transform.local_transform.position.y - layer.transform.local_transform.anchor_point.y * scale_y)) - origin_y;

        const int width = std::max(1, static_cast<int>(std::lround(static_cast<float>(layer.transform.width) * scale_x)));
        const int height = std::max(1, static_cast<int>(std::lround(static_cast<float>(layer.transform.height) * scale_y)));
        surface->fill_rect(RectI{base_x, base_y, width, height}, to_color(layer.text.fill_color), true);
        return true;
    }
};

class ProceduralLayerRenderer : public ILayerRenderer {
public:
    bool render(
        const scene::EvaluatedLayerState& layer,
        const scene::EvaluatedCompositionState& state,
        const render::RenderIntent& intent,
        RenderContext& context,
        const std::optional<RectI>& target_rect,
        std::shared_ptr<SurfaceRGBA>& surface) const override {
        (void)intent;
        (void)state; (void)context;
        render_procedural_layer(*surface, layer, layer.playback.local_time_seconds, target_rect);
        return true;
    }
};

} // namespace

void LayerRendererRegistry::register_builtin_renderers() {
#ifdef TACHYON_ENABLE_MEDIA
    register_renderer(LayerType::Image, std::make_unique<ImageLayerRenderer>());
#endif
    register_renderer(LayerType::Solid, std::make_unique<SolidLayerRenderer>());
#ifdef TACHYON_ENABLE_MEDIA
    register_renderer(LayerType::Video, std::make_unique<VideoLayerRenderer>());
#endif
#ifdef TACHYON_ENABLE_TEXT
    register_renderer(LayerType::Text, std::make_unique<TextLayerRenderer>());
#endif
    register_renderer(LayerType::Procedural, std::make_unique<ProceduralLayerRenderer>());
}

std::shared_ptr<SurfaceRGBA> render_precomp_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    const render::RenderIntent& intent,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    RenderContext& context) {

    if (context.precomp_cache && layer.nested_composition) {
        const std::string cache_key = make_precomp_cache_key(layer, state, task);
        if (auto cached = context.precomp_cache->get(cache_key)) {
            return std::const_pointer_cast<SurfaceRGBA>(cached);
        }

        RendererResourceProvider provider(context);
        auto intent_result = render::build_render_intent(*layer.nested_composition, &provider);
        renderer2d::EffectRegistry effect_reg;
        auto nested = render_evaluated_composition_2d(*layer.nested_composition, intent_result.intent, plan, task, context, effect_reg);
        if (nested.surface) {
            auto surface_copy = std::make_shared<SurfaceRGBA>(*nested.surface);
            context.precomp_cache->put(cache_key, surface_copy);
            return surface_copy;
        }
        return nested.surface;
    }

    if (!state.layers.empty() && layer.nested_composition) {
        RendererResourceProvider provider(context);
        auto intent_result = render::build_render_intent(*layer.nested_composition, &provider);
        renderer2d::EffectRegistry effect_reg;
        auto nested = render_evaluated_composition_2d(*layer.nested_composition, intent_result.intent, plan, task, context, effect_reg);
        return nested.surface;
    }
    return make_canvas(state, std::nullopt, context);
}

std::shared_ptr<SurfaceRGBA> render_simple_layer_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    const render::RenderIntent& intent,
    RenderContext& context,
    const std::optional<RectI>& target_rect) {

    auto surface = make_canvas(state, target_rect, context);
    if (!surface) {
        return surface;
    }

    auto* renderer = LayerRendererRegistry::get().get_renderer(layer.identity.type);
    if (renderer) {
        renderer->render(layer, state, intent, context, target_rect, surface);
    }

    return surface;
}

} // namespace tachyon::renderer2d
