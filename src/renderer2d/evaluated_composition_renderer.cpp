#include "tachyon/renderer2d/evaluated_composition_renderer.h"

#include "tachyon/renderer2d/color/blending.h"
#include "tachyon/renderer2d/effect_host.h"
#include "tachyon/renderer2d/evaluated_composition/mask_renderer.h"
#include "tachyon/renderer2d/framebuffer.h"
#include "tachyon/renderer2d/rasterizer.h"
#include "tachyon/renderer3d/ray_tracer.h"
#include "tachyon/media/media_manager.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include "tachyon/runtime/execution/tile_scheduler.h"

namespace tachyon {
namespace {

renderer2d::Color from_color_spec(const ColorSpec& spec) {
    return renderer2d::Color{
        static_cast<float>(spec.r) / 255.0f,
        static_cast<float>(spec.g) / 255.0f,
        static_cast<float>(spec.b) / 255.0f,
        static_cast<float>(spec.a) / 255.0f
    };
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
            // Values are 0-1, so we don't need to normalize.
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
            } else {
                const auto dest = dst.try_get_pixel(ux, uy);
                if (dest) {
                    const renderer2d::Color blended = renderer2d::blend_mode_color(pixel, *dest, blend_mode);
                    // Standard AE behavior: The blend mode result is then composited over the destination
                    // using the source's alpha (which is already included in 'blended.a').
                    dst.set_pixel(ux, uy, renderer2d::detail::composite_src_over_linear(blended, *dest));
                }
            }
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

renderer2d::EffectHost& effect_host_for(renderer2d::RenderContext2D& context) {
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
        params.colors.emplace(key, renderer2d::Color{
            static_cast<float>(value.r) / 255.0f,
            static_cast<float>(value.g) / 255.0f,
            static_cast<float>(value.b) / 255.0f,
            static_cast<float>(value.a) / 255.0f
        });
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

std::shared_ptr<renderer2d::SurfaceRGBA> render_layer_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    renderer2d::RenderContext2D& context,
    const std::optional<renderer2d::RectI>& target_rect = std::nullopt);

std::shared_ptr<renderer2d::SurfaceRGBA> render_precomp_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    renderer2d::RenderContext2D& context) {

    if (!layer.nested_composition) {
        return make_surface(state.width, state.height, context);
    }

    const std::string cache_key = layer.precomp_id.value_or(layer.id) + ":" + std::to_string(task.frame_number);
    if (context.precomp_cache) {
        if (const auto cached = context.precomp_cache->lookup(cache_key)) {
            // Precomp surfaces are read-only at the cache boundary, but the renderer
            // pipeline consumes a mutable shared_ptr for downstream compatibility.
            auto mutable_cached = std::const_pointer_cast<renderer2d::SurfaceRGBA>(cached);
            return mutable_cached;
        }
    }

    const RasterizedFrame2D nested = render_evaluated_composition_2d(*layer.nested_composition, plan, task, context);
    std::shared_ptr<renderer2d::SurfaceRGBA> surface = nested.surface 
        ? nested.surface 
        : make_surface(layer.nested_composition->width, layer.nested_composition->height, context);

    if (context.precomp_cache) {
        context.precomp_cache->store(cache_key, surface);
    }
    return surface;
}

std::shared_ptr<renderer2d::SurfaceRGBA> render_simple_layer_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    renderer2d::RenderContext2D& context,
    const std::optional<renderer2d::RectI>& target_rect = std::nullopt) {

    const std::int64_t w = target_rect ? target_rect->width : static_cast<std::int64_t>(state.width * context.policy.resolution_scale);
    const std::int64_t h = target_rect ? target_rect->height : static_cast<std::int64_t>(state.height * context.policy.resolution_scale);
    
    std::shared_ptr<renderer2d::SurfaceRGBA> surface = make_surface(w, h, context);

    if (!layer.visible || !layer.enabled || !layer.active) {
        return surface;
    }

    if (layer.type == scene::LayerType::Shape && layer.shape_path.has_value()) {
        const auto& sp = *layer.shape_path;
        renderer2d::PathGeometry geom;
        if (!sp.points.empty()) {
            geom.commands.push_back({renderer2d::PathVerb::MoveTo, sp.points[0].position});
            for (std::size_t i = 1; i < sp.points.size(); ++i) {
                const auto& prev = sp.points[i-1];
                const auto& curr = sp.points[i];
                if (prev.tangent_out.length_squared() > 1e-6f || curr.tangent_in.length_squared() > 1e-6f) {
                    geom.commands.push_back({renderer2d::PathVerb::CubicTo, prev.position + prev.tangent_out, curr.position + curr.tangent_in, curr.position});
                } else {
                    geom.commands.push_back({renderer2d::PathVerb::LineTo, curr.position});
                }
            }
            if (sp.closed) {
                geom.commands.push_back({renderer2d::PathVerb::Close});
            }
        }

        if (target_rect) {
            for (auto& cmd : geom.commands) {
                cmd.p0.x -= static_cast<float>(target_rect->x);
                cmd.p0.y -= static_cast<float>(target_rect->y);
                cmd.p1.x -= static_cast<float>(target_rect->x);
                cmd.p1.y -= static_cast<float>(target_rect->y);
                cmd.p2.x -= static_cast<float>(target_rect->x);
                cmd.p2.y -= static_cast<float>(target_rect->y);
            }
        }

        // Fill
        if (layer.fill_color.a > 0 || layer.gradient_fill.has_value()) {
            renderer2d::FillPathStyle style;
            style.fill_color = from_color_spec(layer.fill_color);
            style.gradient = layer.gradient_fill;
            style.opacity = static_cast<float>(layer.opacity);
            renderer2d::PathRasterizer::fill(*surface, geom, style);
        }

        // Stroke
        if (layer.stroke_width > 0.0f && (layer.stroke_color.a > 0 || layer.gradient_stroke.has_value())) {
            renderer2d::StrokePathStyle style;
            style.stroke_color = from_color_spec(layer.stroke_color);
            style.gradient = layer.gradient_stroke;
            style.stroke_width = layer.stroke_width;
            style.opacity = static_cast<float>(layer.opacity);
            style.cap = layer.line_cap;
            style.join = layer.line_join;
            style.miter_limit = layer.miter_limit;
            renderer2d::PathRasterizer::stroke(*surface, geom, style);
        }
    } else if (layer.type == scene::LayerType::Image && context.media_manager && layer.asset_path.has_value()) {
        const auto* texture = context.media_manager->get_image(*layer.asset_path);
        if (texture) {
            const float tex_w = static_cast<float>(layer.width > 0 ? layer.width : texture->width());
            const float tex_h = static_cast<float>(layer.height > 0 ? layer.height : texture->height());
            
            // Transform corners from layer space [0,w] x [0,h] to screen space
            auto transform_point = [&](float x, float y) -> renderer2d::TexturedVertex2D {
                // Layer space point to world/screen space using world_matrix
                math::Vector3 p{x, y, 0.0f};
                math::Vector3 tp = math::transform_point(layer.world_matrix, p);
                
                // For 2D rendering, we assume the Z component is already handled or negligible
                // In AE, (0,0) is top-left of the comp.
                return renderer2d::TexturedVertex2D{
                    tp.x, tp.y,
                    x / tex_w, y / tex_h,
                    1.0f // inv_w for 2D is 1.0
                };
            };

            renderer2d::TexturedVertex2D v0 = transform_point(0.0f, 0.0f);
            renderer2d::TexturedVertex2D v1 = transform_point(tex_w, 0.0f);
            renderer2d::TexturedVertex2D v2 = transform_point(tex_w, tex_h);
            renderer2d::TexturedVertex2D v3 = transform_point(0.0f, tex_h);

            renderer2d::TexturedQuadPrimitive quad = renderer2d::TexturedQuadPrimitive::custom(
                v0, v1, v2, v3, texture, from_color_spec(layer.fill_color)
            );
            if (target_rect) {
                for (auto& v : quad.vertices) {
                    v.x -= static_cast<float>(target_rect->x);
                    v.y -= static_cast<float>(target_rect->y);
                }
            }
            renderer2d::CPURasterizer::draw_textured_quad(*surface, quad);
        }
    } else {
        renderer2d::RectI rect = layer_rect(layer, state.width, state.height, context.policy.resolution_scale);
        if (target_rect) {
            rect.x -= target_rect->x;
            rect.y -= target_rect->y;
        }
        if (rect.width > 0 && rect.height > 0) {
            renderer2d::Color color = from_color_spec(layer.fill_color);
            color = apply_opacity(color, layer.opacity);
            surface->fill_rect(rect, color, true);
        }
    }

    return surface;
}

std::shared_ptr<renderer2d::SurfaceRGBA> render_layer_surface(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    renderer2d::RenderContext2D& context,
    const std::optional<renderer2d::RectI>& target_rect) {

    switch (layer.type) {
    case scene::LayerType::Precomp:
        return render_precomp_surface(layer, state, plan, task, context); // Precomps might need tiling too, but let's start with layers
    case ::tachyon::scene::LayerType::NullLayer: {
        return make_surface(state.width, state.height, context);
    }
    case scene::LayerType::Mask:
        return render_simple_layer_surface(layer, state, context, target_rect);
    case scene::LayerType::Solid:
    case scene::LayerType::Shape:
    case scene::LayerType::Image:
    case scene::LayerType::Text:
    default:
        return render_simple_layer_surface(layer, state, context, target_rect);
    }
}

} // namespace

RasterizedFrame2D render_evaluated_composition_2d(
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    renderer2d::RenderContext2D& context) {

    RasterizedFrame2D frame;
    frame.frame_number = task.frame_number;
    frame.width = state.width;
    frame.height = state.height;
    frame.layer_count = state.layers.size();
    frame.backend_name = "cpu-evaluated-composition";
    frame.cache_key = task.cache_key.value;
    frame.note = "Frame rasterized from evaluated composition";
    const float res_scale = plan.quality_policy.resolution_scale;
    const std::uint32_t working_width = static_cast<std::uint32_t>(std::max<std::int64_t>(1, static_cast<std::int64_t>(std::round(static_cast<float>(state.width) * res_scale))));
    const std::uint32_t working_height = static_cast<std::uint32_t>(std::max<std::int64_t>(1, static_cast<std::int64_t>(std::round(static_cast<float>(state.height) * res_scale))));

    frame.surface = std::make_shared<renderer2d::SurfaceRGBA>(working_width, working_height);

    if (!frame.surface) {
        return frame;
    }

    auto& dst = *frame.surface;
    dst.clear(renderer2d::Color::transparent());

    renderer2d::EffectHost& host = effect_host_for(context);

    // Identify if we have 3D layers and trigger 3D pass if available
    bool has_any_3d = std::any_of(state.layers.begin(), state.layers.end(), [](const auto& l) { return l.is_3d && l.visible; });
    if (has_any_3d && !context.ray_tracer) {
        context.ray_tracer = std::make_shared<renderer3d::RayTracer>();
    }

    auto render_pass = [&](renderer2d::SurfaceRGBA& target_surface, const std::optional<renderer2d::RectI>& tile_rect = std::nullopt) {
        // AE Rendering Order: Bottom to Top
        for (std::size_t i = 0; i < state.layers.size(); ++i) {
            const auto& layer = state.layers[i];
            if (!layer.enabled || !layer.active) {
                continue;
            }

            if (layer.is_3d && layer.visible) {
                if (tile_rect) {
                    // Skip 3D blocks in tiled mode for now, or we'd need to handle 3D sub-rect rendering
                    continue; 
                }
                // Found a 3D block. Collect contiguous 3D layers.
                std::vector<std::size_t> block_indices;
                block_indices.push_back(i);
                std::size_t last_block_idx = i;
                for (std::size_t j = i + 1; j < state.layers.size(); ++j) {
                    if (state.layers[j].is_3d && state.layers[j].visible && state.layers[j].enabled && state.layers[j].active) {
                        block_indices.push_back(j);
                        last_block_idx = j;
                    } else if (state.layers[j].enabled && state.layers[j].active) {
                        break;
                    }
                }

                // Render the 3D block
                context.ray_tracer->set_samples_per_pixel(context.policy.ray_tracer_spp);
                context.ray_tracer->build_scene_subset(state, block_indices, context.font);
                
                auto world_3d = make_surface(state.width, state.height, context);
                std::vector<float> hdr_buffer(static_cast<std::size_t>(state.width) * state.height * 4, 0.0f);
                context.ray_tracer->render(state, hdr_buffer.data(), nullptr, static_cast<int>(state.width), static_cast<int>(state.height));
                
                for (std::int64_t px_idx = 0; px_idx < state.width * state.height; ++px_idx) {
                    const float r = hdr_buffer[px_idx * 4 + 0];
                    const float g = hdr_buffer[px_idx * 4 + 1];
                    const float b = hdr_buffer[px_idx * 4 + 2];
                    const float a = hdr_buffer[px_idx * 4 + 3];
                    world_3d->set_pixel(static_cast<std::uint32_t>(px_idx % state.width), static_cast<std::uint32_t>(px_idx / state.width), {r, g, b, a});
                }

                composite_surface(target_surface, *world_3d, 0, 0, renderer2d::BlendMode::Normal);
                i = last_block_idx;
                continue;
            }

            auto layer_surface = render_layer_surface(layer, state, plan, task, context, tile_rect);

            int ox = 0;
            int oy = 0;
            if (tile_rect) {
                ox = tile_rect->x;
                oy = tile_rect->y;
            }

            if (layer.is_adjustment_layer) {
                if (context.policy.effects_enabled) {
                    auto adjusted = apply_effect_pipeline(target_surface, layer.effects, host);
                    multiply_surface_alpha(adjusted, static_cast<float>(layer.opacity));
                    composite_surface(target_surface, adjusted, 0, 0, renderer2d::BlendMode::Normal);
                }
                continue;
            }

            if (layer.track_matte_layer_index.has_value() && layer.track_matte_type != TrackMatteType::None) {
                const std::size_t matte_index = *layer.track_matte_layer_index;
                if (matte_index < state.layers.size()) {
                    auto matte_surface = render_layer_surface(state.layers[matte_index], state, plan, task, context, tile_rect);
                    apply_mask(*layer_surface, *matte_surface, layer.track_matte_type);
                }
            }

            auto parse_blend_mode = [](const std::string& mode_str) -> renderer2d::BlendMode {
                std::string s = mode_str;
                std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
                if (s == "additive" || s == "add") return renderer2d::BlendMode::Additive;
                if (s == "multiply") return renderer2d::BlendMode::Multiply;
                if (s == "screen") return renderer2d::BlendMode::Screen;
                if (s == "overlay") return renderer2d::BlendMode::Overlay;
                if (s == "softlight" || s == "soft_light") return renderer2d::BlendMode::SoftLight;
                if (s == "hardlight" || s == "hard_light") return renderer2d::BlendMode::HardLight;
                if (s == "vividlight" || s == "vivid_light") return renderer2d::BlendMode::VividLight;
                if (s == "linearlight" || s == "linear_light") return renderer2d::BlendMode::LinearLight;
                if (s == "pinlight" || s == "pin_light") return renderer2d::BlendMode::PinLight;
                if (s == "hardmix" || s == "hard_mix") return renderer2d::BlendMode::HardMix;
                if (s == "difference") return renderer2d::BlendMode::Difference;
                if (s == "exclusion") return renderer2d::BlendMode::Exclusion;
                if (s == "subtract") return renderer2d::BlendMode::Subtract;
                if (s == "divide") return renderer2d::BlendMode::Divide;
                if (s == "darken") return renderer2d::BlendMode::Darken;
                if (s == "lighten") return renderer2d::BlendMode::Lighten;
                if (s == "colordodge" || s == "color_dodge") return renderer2d::BlendMode::ColorDodge;
                if (s == "colorburn" || s == "color_burn") return renderer2d::BlendMode::ColorBurn;
                if (s == "lineardodge" || s == "linear_dodge") return renderer2d::BlendMode::LinearDodge;
                if (s == "linearburn" || s == "linear_burn") return renderer2d::BlendMode::LinearBurn;
                if (s == "darkercolor" || s == "darker_color") return renderer2d::BlendMode::DarkerColor;
                if (s == "lightercolor" || s == "lighter_color") return renderer2d::BlendMode::LighterColor;
                if (s == "hue") return renderer2d::BlendMode::Hue;
                if (s == "saturation") return renderer2d::BlendMode::Saturation;
                if (s == "color") return renderer2d::BlendMode::Color;
                if (s == "luminosity") return renderer2d::BlendMode::Luminosity;
                return renderer2d::BlendMode::Normal;
            };

            composite_surface(target_surface, *layer_surface, ox, oy, parse_blend_mode(layer.blend_mode));
        }
    };

    const int tile_size = plan.quality_policy.tile_size;
    if (tile_size > 0) {
        TileGrid grid = build_tile_grid(state, tile_size);
        for (const auto& tile_roi : grid.tiles) {
            const int x0 = static_cast<int>(std::floor(static_cast<float>(tile_roi.x) * res_scale));
            const int y0 = static_cast<int>(std::floor(static_cast<float>(tile_roi.y) * res_scale));
            const int x1 = static_cast<int>(std::floor(static_cast<float>(tile_roi.x + tile_roi.width) * res_scale));
            const int y1 = static_cast<int>(std::floor(static_cast<float>(tile_roi.y + tile_roi.height) * res_scale));
            
            renderer2d::RectI working_tile{x0, y0, x1 - x0, y1 - y0};
            
            dst.set_clip_rect(working_tile);
            render_pass(dst, working_tile);
        }
        dst.reset_clip_rect();
    } else {
        render_pass(dst);
    }

    std::vector<float> accum_r(static_cast<std::size_t>(dst.width()) * dst.height(), 0.0f);
    std::vector<float> accum_g = accum_r;
    std::vector<float> accum_b = accum_r;
    std::vector<float> accum_a = accum_r;

    for (std::uint32_t y = 0; y < dst.height(); ++y) {
        for (std::uint32_t x = 0; x < dst.width(); ++x) {
            const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(dst.width()) + static_cast<std::size_t>(x);
            const renderer2d::Color px = dst.get_pixel(x, y);
            accum_r[index] = px.r;
            accum_g[index] = px.g;
            accum_b[index] = px.b;
            accum_a[index] = px.a;
        }
    }

    renderer2d::MaskRenderer::applyMask(state, context, accum_r, accum_g, accum_b, accum_a);
    for (std::uint32_t y = 0; y < dst.height(); ++y) {
        for (std::uint32_t x = 0; x < dst.width(); ++x) {
            const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(dst.width()) + static_cast<std::size_t>(x);
            dst.set_pixel(x, y, renderer2d::Color{
                accum_r[index],
                accum_g[index],
                accum_b[index],
                accum_a[index]
            });
        }
    }

    return frame;
}

} // namespace tachyon
