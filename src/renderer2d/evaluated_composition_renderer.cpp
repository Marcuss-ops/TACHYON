#include "tachyon/renderer2d/evaluated_composition_renderer.h"

#include "tachyon/renderer2d/effect_host.h"
#include "tachyon/renderer2d/path_rasterizer.h"
#include "tachyon/renderer2d/rasterizer_ops.h"
#include "tachyon/media/image_manager.h"
#include "tachyon/runtime/tile_scheduler.h"
#include "tachyon/text/font.h"
#include "tachyon/text/layout.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <map>

namespace tachyon {
namespace {

enum class RenderLayerKind {
    Solid,
    Shape,
    Mask,
    Text,
    Image,
    Precomp,
    Camera,
    Unknown
};

struct RenderLayerView {
    RenderLayerKind kind{RenderLayerKind::Unknown};
    bool visible{false};
    int z_order{0};
    std::size_t layer_index{0};
    float opacity{1.0f};
    math::Vector2 position{math::Vector2::zero()};
    math::Vector2 scale{math::Vector2::one()};
    std::string id;
    std::string label;
    std::optional<scene::EvaluatedShapePath> shape_path;
    
    // Metadata
    std::int64_t width{0};
    std::int64_t height{0};
    float stroke_width{1.0f};
    std::string text_content;
    renderer2d::Color fill_color{renderer2d::Color::white()};
    renderer2d::Color stroke_color{renderer2d::Color::black()};
    std::vector<EffectSpec> effects;

    // Mask / Precomp data
    TrackMatteType track_matte_type{TrackMatteType::None};
    std::optional<std::size_t> track_matte_layer_index;
    std::optional<std::string> precomp_id;
    std::shared_ptr<scene::EvaluatedCompositionState> nested_composition;
};

renderer2d::Color premultiply_color(renderer2d::Color color) {
    color.r = static_cast<std::uint8_t>((static_cast<std::uint32_t>(color.r) * color.a + 127U) / 255U);
    color.g = static_cast<std::uint8_t>((static_cast<std::uint32_t>(color.g) * color.a + 127U) / 255U);
    color.b = static_cast<std::uint8_t>((static_cast<std::uint32_t>(color.b) * color.a + 127U) / 255U);
    return color;
}

renderer2d::Color color_with_opacity(renderer2d::Color base, float opacity) {
    const float clamped = std::clamp(opacity, 0.0f, 1.0f);
    base.a = static_cast<std::uint8_t>(std::round(static_cast<float>(base.a) * clamped));
    return premultiply_color(base);
}

renderer2d::Color from_spec(const ColorSpec& spec) {
    return renderer2d::Color{spec.r, spec.g, spec.b, spec.a};
}

renderer2d::RectI intersect_rects(const renderer2d::RectI& a, const renderer2d::RectI& b) {
    const int x0 = std::max(a.x, b.x);
    const int y0 = std::max(a.y, b.y);
    const int x1 = std::min(a.x + a.width, b.x + b.width);
    const int y1 = std::min(a.y + a.height, b.y + b.height);
    return renderer2d::RectI{x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)};
}

renderer2d::RectI full_rect(std::int64_t width, std::int64_t height) {
    return renderer2d::RectI{0, 0, static_cast<int>(width), static_cast<int>(height)};
}

renderer2d::RectI inflate_rect(const renderer2d::RectI& rect, int margin, std::int64_t width, std::int64_t height) {
    const int x0 = std::max(0, rect.x - margin);
    const int y0 = std::max(0, rect.y - margin);
    const int x1 = std::min(static_cast<int>(width), rect.x + rect.width + margin);
    const int y1 = std::min(static_cast<int>(height), rect.y + rect.height + margin);
    return renderer2d::RectI{x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)};
}

renderer2d::RectI layer_bounds(const RenderLayerView& layer, std::int64_t composition_width, std::int64_t composition_height) {
    int base_w = static_cast<int>(layer.width);
    int base_h = static_cast<int>(layer.height);

    if (base_w == 0 || base_h == 0) {
        if (layer.kind == RenderLayerKind::Text) {
            base_w = std::max(64, static_cast<int>(composition_width / 6));
            base_h = std::max(32, static_cast<int>(composition_height / 10));
        } else if (layer.kind == RenderLayerKind::Solid || layer.kind == RenderLayerKind::Shape) {
            base_w = std::max(64, static_cast<int>(composition_width / 4));
            base_h = std::max(32, static_cast<int>(composition_height / 8));
        } else if (layer.kind == RenderLayerKind::Precomp && layer.nested_composition) {
            base_w = static_cast<int>(layer.nested_composition->width);
            base_h = static_cast<int>(layer.nested_composition->height);
        } else {
            base_w = 100;
            base_h = 100;
        }
    }

    const int width = std::max(1, static_cast<int>(std::round(static_cast<float>(base_w) * layer.scale.x)));
    const int height = std::max(1, static_cast<int>(std::round(static_cast<float>(base_h) * layer.scale.y)));
    return renderer2d::RectI{
        static_cast<int>(std::round(layer.position.x)),
        static_cast<int>(std::round(layer.position.y)),
        width,
        height
    };
}

renderer2d::PathGeometry build_shape_path(const RenderLayerView& layer) {
    renderer2d::PathGeometry path;
    if (!layer.shape_path.has_value() || layer.shape_path->points.empty()) {
        return path;
    }

    for (std::size_t index = 0; index < layer.shape_path->points.size(); ++index) {
        const auto& point = layer.shape_path->points[index];
        const float x = layer.position.x + point.position.x * layer.scale.x;
        const float y = layer.position.y + point.position.y * layer.scale.y;
        
        if (index == 0U) {
            path.commands.push_back(renderer2d::PathCommand{renderer2d::PathVerb::MoveTo, {x, y}});
        } else {
            const auto& prev = layer.shape_path->points[index - 1];
            if (prev.tangent_out.x == 0.0f && prev.tangent_out.y == 0.0f && 
                point.tangent_in.x == 0.0f && point.tangent_in.y == 0.0f) {
                path.commands.push_back(renderer2d::PathCommand{renderer2d::PathVerb::LineTo, {x, y}});
            } else {
                const float cp1x = layer.position.x + (prev.position.x + prev.tangent_out.x) * layer.scale.x;
                const float cp1y = layer.position.y + (prev.position.y + prev.tangent_out.y) * layer.scale.y;
                const float cp2x = layer.position.x + (point.position.x + point.tangent_in.x) * layer.scale.x;
                const float cp2y = layer.position.y + (point.position.y + point.tangent_in.y) * layer.scale.y;
                path.commands.push_back(renderer2d::PathCommand{
                    renderer2d::PathVerb::CubicTo, 
                    {cp1x, cp1y}, 
                    {cp2x, cp2y}, 
                    {x, y}
                });
            }
        }
    }

    if (layer.shape_path->closed) {
        path.commands.push_back(renderer2d::PathCommand{renderer2d::PathVerb::Close});
    }

    return path;
}

renderer2d::PathGeometry translate_path(const renderer2d::PathGeometry& path, int dx, int dy) {
    renderer2d::PathGeometry translated;
    translated.commands.reserve(path.commands.size());
    for (const auto& command : path.commands) {
        renderer2d::PathCommand copy = command;
        copy.p0.x -= static_cast<float>(dx);
        copy.p0.y -= static_cast<float>(dy);
        copy.p1.x -= static_cast<float>(dx);
        copy.p1.y -= static_cast<float>(dy);
        copy.p2.x -= static_cast<float>(dx);
        copy.p2.y -= static_cast<float>(dy);
        translated.commands.push_back(copy);
    }
    return translated;
}

RenderLayerView to_view(const scene::EvaluatedLayerState& layer) {
    RenderLayerView view;
    view.visible = layer.visible;
    view.z_order = static_cast<int>(layer.layer_index);
    view.layer_index = layer.layer_index;
    view.opacity = static_cast<float>(layer.opacity);
    view.position = layer.position;
    view.scale = layer.scale;
    view.id = layer.id;
    view.label = layer.name.empty() ? layer.id : layer.name;
    view.shape_path = layer.shape_path;
    view.track_matte_type = layer.track_matte_type;
    view.track_matte_layer_index = layer.track_matte_layer_index;
    view.precomp_id = layer.precomp_id;
    view.nested_composition = layer.nested_composition;
    
    view.width = layer.width;
    view.height = layer.height;
    view.stroke_width = static_cast<float>(layer.stroke_width);
    view.text_content = layer.text_content;
    view.fill_color = from_spec(layer.fill_color);
    view.stroke_color = from_spec(layer.stroke_color);
    view.effects = layer.effects;

    if (layer.type == "solid") view.kind = RenderLayerKind::Solid;
    else if (layer.type == "shape") view.kind = RenderLayerKind::Shape;
    else if (layer.type == "mask") view.kind = RenderLayerKind::Mask;
    else if (layer.type == "text") view.kind = RenderLayerKind::Text;
    else if (layer.type == "image") view.kind = RenderLayerKind::Image;
    else if (layer.precomp_id.has_value()) view.kind = RenderLayerKind::Precomp;
    else if (layer.is_camera) view.kind = RenderLayerKind::Camera;
    else view.kind = RenderLayerKind::Unknown;

    return view;
}

// ... existing find_default_font_path and default_font ...
std::filesystem::path find_default_font_path() {
    const std::filesystem::path candidates[] = {
        "Tachyon/tests/fixtures/fonts/minimal_5x7.bdf",
        "tests/fixtures/fonts/minimal_5x7.bdf",
        "../tests/fixtures/fonts/minimal_5x7.bdf",
        "../../tests/fixtures/fonts/minimal_5x7.bdf",
        "../../../tests/fixtures/fonts/minimal_5x7.bdf",
        "../Tachyon/tests/fixtures/fonts/minimal_5x7.bdf",
        "../../Tachyon/tests/fixtures/fonts/minimal_5x7.bdf",
        "../../../Tachyon/tests/fixtures/fonts/minimal_5x7.bdf"
    };

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return {};
}

const text::BitmapFont* default_font() {
    static text::BitmapFont font;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        const std::filesystem::path path = find_default_font_path();
        if (!path.empty()) {
            font.load_bdf(path);
        }
    }
    return font.is_loaded() ? &font : nullptr;
}

struct IdentityEffect final : renderer2d::Effect {
    renderer2d::SurfaceRGBA apply(const renderer2d::SurfaceRGBA& input, const renderer2d::EffectParams&) const override {
        return input;
    }
};

renderer2d::EffectHost& scene_effect_host() {
    static renderer2d::EffectHost host;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        host.register_effect("identity", std::make_unique<IdentityEffect>());
        host.register_effect("blur", std::make_unique<renderer2d::GaussianBlurEffect>());
        host.register_effect("shadow", std::make_unique<renderer2d::DropShadowEffect>());
        host.register_effect("fill", std::make_unique<renderer2d::FillEffect>());
        host.register_effect("tint", std::make_unique<renderer2d::TintEffect>());
    }
    return host;
}

void apply_track_matte(renderer2d::SurfaceRGBA& target, const renderer2d::SurfaceRGBA& matte_source, TrackMatteType type) {
    if (type == TrackMatteType::None) return;

    for (uint32_t y = 0; y < target.height(); ++y) {
        for (uint32_t x = 0; x < target.width(); ++x) {
            renderer2d::Color pixel = target.get_pixel(x, y);
            renderer2d::Color matte_pixel = matte_source.get_pixel(x, y);

            float factor = 0.0f;
            switch (type) {
                case TrackMatteType::Alpha:
                    factor = static_cast<float>(matte_pixel.a) / 255.0f;
                    break;
                case TrackMatteType::AlphaInverted:
                    factor = 1.0f - (static_cast<float>(matte_pixel.a) / 255.0f);
                    break;
                case TrackMatteType::Luma:
                    factor = (0.299f * matte_pixel.r + 0.587f * matte_pixel.g + 0.114f * matte_pixel.b) / 255.0f;
                    break;
                case TrackMatteType::LumaInverted:
                    factor = 1.0f - ((0.299f * matte_pixel.r + 0.587f * matte_pixel.g + 0.114f * matte_pixel.b) / 255.0f);
                    break;
                default: break;
            }

            pixel.a = static_cast<uint8_t>(std::round(static_cast<float>(pixel.a) * factor));
            pixel.r = static_cast<uint8_t>(std::round(static_cast<float>(pixel.r) * factor));
            pixel.g = static_cast<uint8_t>(std::round(static_cast<float>(pixel.g) * factor));
            pixel.b = static_cast<uint8_t>(std::round(static_cast<float>(pixel.b) * factor));
            
            target.set_pixel(x, y, pixel);
        }
    }
}

void composite_surface(renderer2d::SurfaceRGBA& destination, const renderer2d::SurfaceRGBA& source, const renderer2d::RectI& clip) {
    const renderer2d::RectI bounds = intersect_rects(clip, renderer2d::RectI{0, 0, static_cast<int>(destination.width()), static_cast<int>(destination.height())});
    if (bounds.width <= 0 || bounds.height <= 0) {
        return;
    }

    for (int y = bounds.y; y < bounds.y + bounds.height; ++y) {
        for (int x = bounds.x; x < bounds.x + bounds.width; ++x) {
            const renderer2d::Color pixel = source.get_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y));
            if (pixel.a != 0U) {
                destination.blend_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), pixel);
            }
        }
    }
}

void composite_surface_at(
    renderer2d::SurfaceRGBA& destination,
    const renderer2d::SurfaceRGBA& source,
    int offset_x,
    int offset_y,
    const renderer2d::RectI& clip) {

    const renderer2d::RectI bounds = intersect_rects(
        clip,
        renderer2d::RectI{offset_x, offset_y, static_cast<int>(source.width()), static_cast<int>(source.height())});
    if (bounds.width <= 0 || bounds.height <= 0) {
        return;
    }

    for (int y = bounds.y; y < bounds.y + bounds.height; ++y) {
        for (int x = bounds.x; x < bounds.x + bounds.width; ++x) {
            const renderer2d::Color pixel = source.get_pixel(static_cast<std::uint32_t>(x - offset_x), static_cast<std::uint32_t>(y - offset_y));
            if (pixel.a != 0U) {
                destination.blend_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), pixel);
            }
        }
    }
}

thread_local int g_precomp_recursion_depth = 0;

RasterizedFrame2D render_composition_recursive(
    const std::vector<scene::EvaluatedLayerState>& layers,
    std::int64_t width,
    std::int64_t height,
    const RenderPlan& plan,
    const FrameRenderTask& task);

void render_layer_to_surface(
    renderer2d::SurfaceRGBA& layer_surface,
    const RenderLayerView& layer,
    std::int64_t composition_width,
    std::int64_t composition_height,
    const renderer2d::RectI& render_region,
    const RenderPlan& plan,
    const FrameRenderTask& task) {

    const renderer2d::RectI bounds = layer_bounds(layer, composition_width, composition_height);
    const renderer2d::RectI clipped = intersect_rects(bounds, render_region);
    if (clipped.width <= 0 || clipped.height <= 0) {
        return;
    }
    const renderer2d::RectI local_clip{
        clipped.x - render_region.x,
        clipped.y - render_region.y,
        clipped.width,
        clipped.height
    };
    layer_surface.set_clip_rect(local_clip);

    switch (layer.kind) {
        case RenderLayerKind::Solid: {
            const renderer2d::Color color = color_with_opacity(layer.fill_color, layer.opacity);
            layer_surface.fill_rect(renderer2d::RectI{local_clip.x, local_clip.y, local_clip.width, local_clip.height}, color);
            break;
        }
        case RenderLayerKind::Shape: {
            renderer2d::PathGeometry path = build_shape_path(layer);
            if (path.commands.empty()) {
                const int local_x = bounds.x - render_region.x;
                const int local_y = bounds.y - render_region.y;
                path.commands = {
                    {renderer2d::PathVerb::MoveTo, {static_cast<float>(local_x), static_cast<float>(local_y)}},
                    {renderer2d::PathVerb::LineTo, {static_cast<float>(local_x + clipped.width), static_cast<float>(local_y)}},
                    {renderer2d::PathVerb::LineTo, {static_cast<float>(local_x + clipped.width), static_cast<float>(local_y + clipped.height)}},
                    {renderer2d::PathVerb::LineTo, {static_cast<float>(local_x), static_cast<float>(local_y + clipped.height)}},
                    {renderer2d::PathVerb::Close}
                };
            } else {
                path = translate_path(path, render_region.x, render_region.y);
            }

            renderer2d::FillPathStyle fill_style;
            fill_style.fill_color = color_with_opacity(layer.fill_color, layer.opacity);
            fill_style.opacity = 1.0f;
            renderer2d::PathRasterizer::fill(layer_surface, path, fill_style);

            renderer2d::StrokePathStyle stroke_style;
            stroke_style.stroke_color = color_with_opacity(layer.stroke_color, layer.opacity);
            stroke_style.stroke_width = layer.stroke_width;
            stroke_style.opacity = 1.0f;
            renderer2d::PathRasterizer::stroke(layer_surface, path, stroke_style);
            break;
        }
        case RenderLayerKind::Text: {
            const text::BitmapFont* font = default_font();
            if (font) {
                const std::string& text_value = layer.text_content.empty() ? layer.label : layer.text_content;
                if (!text_value.empty()) {
                text::TextStyle style;
                style.pixel_size = 14;
                style.fill_color = color_with_opacity(layer.fill_color, layer.opacity);
                text::TextBox box;
                box.width = static_cast<std::uint32_t>(clipped.width);
                box.height = static_cast<std::uint32_t>(clipped.height);
                box.multiline = true;
                const text::TextRasterSurface text_surface = text::rasterize_text_rgba(*font, text_value, style, box, text::TextAlignment::Left);
                const int offset_x = bounds.x - render_region.x;
                const int offset_y = bounds.y - render_region.y;
                for (uint32_t y = 0; y < text_surface.height(); ++y) {
                    for (uint32_t x = 0; x < text_surface.width(); ++x) {
                        const auto pixel = text_surface.get_pixel(x, y);
                        if (pixel.a > 0) {
                            layer_surface.blend_pixel(offset_x + x, offset_y + y, premultiply_color(pixel));
                        }
                    }
                }
                }
            }
            break;
        }
        case RenderLayerKind::Image: {
            const renderer2d::SurfaceRGBA* texture = media::ImageManager::instance().get_image(layer.label);
            if (texture) {
                const int local_x = bounds.x - render_region.x;
                const int local_y = bounds.y - render_region.y;
                for (int y = 0; y < clipped.height; ++y) {
                    for (int x = 0; x < clipped.width; ++x) {
                        uint32_t tx = static_cast<uint32_t>(static_cast<float>(x) / static_cast<float>(std::max(1, clipped.width)) * static_cast<float>(texture->width()));
                        uint32_t ty = static_cast<uint32_t>(static_cast<float>(y) / static_cast<float>(std::max(1, clipped.height)) * static_cast<float>(texture->height()));
                        renderer2d::Color p = texture->get_pixel(tx, ty);
                        p = color_with_opacity(p, layer.opacity);
                        if (p.a > 0) {
                            layer_surface.blend_pixel(local_x + x, local_y + y, p);
                        }
                    }
                }
            }
            break;
        }
        case RenderLayerKind::Precomp: {
            if (layer.nested_composition && g_precomp_recursion_depth < 10) {
                g_precomp_recursion_depth++;
                RasterizedFrame2D nested_frame = render_composition_recursive(layer.nested_composition->layers, layer.nested_composition->width, layer.nested_composition->height, plan, task);
                g_precomp_recursion_depth--;

                if (nested_frame.surface.has_value()) {
                    const auto* texture = &(*nested_frame.surface);
                    const int local_x = bounds.x - render_region.x;
                    const int local_y = bounds.y - render_region.y;
                    for (int y = 0; y < clipped.height; ++y) {
                        for (int x = 0; x < clipped.width; ++x) {
                            uint32_t tx = static_cast<uint32_t>(static_cast<float>(x) / static_cast<float>(std::max(1, clipped.width)) * static_cast<float>(texture->width()));
                            uint32_t ty = static_cast<uint32_t>(static_cast<float>(y) / static_cast<float>(std::max(1, clipped.height)) * static_cast<float>(texture->height()));
                            renderer2d::Color p = texture->get_pixel(tx, ty);
                            p = color_with_opacity(p, layer.opacity);
                            if (p.a > 0) {
                                layer_surface.blend_pixel(local_x + x, local_y + y, p);
                            }
                        }
                    }
                }
            }
            break;
        }
        default: break;
    }

    // Apply effects
    auto& host = scene_effect_host();
    for (const auto& effect_spec : layer.effects) {
        if (!effect_spec.enabled || !host.has_effect(effect_spec.type)) {
            continue;
        }

        renderer2d::EffectParams params;
        for (const auto& [k, v] : effect_spec.scalars) {
            params.scalars[k] = static_cast<float>(v);
        }
        for (const auto& [k, v] : effect_spec.colors) {
            params.colors[k] = from_spec(v);
        }

        layer_surface = host.apply(effect_spec.type, layer_surface, params);
    }

    layer_surface.reset_clip_rect();
}

RasterizedFrame2D render_composition_recursive(
    const std::vector<scene::EvaluatedLayerState>& layers,
    std::int64_t width,
    std::int64_t height,
    const RenderPlan& plan,
    const FrameRenderTask& task) {

    RasterizedFrame2D frame;
    frame.frame_number = task.frame_number;
    frame.width = width;
    frame.height = height;
    frame.surface.emplace(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
    frame.surface->clear(renderer2d::Color::transparent());

    std::vector<RenderLayerView> views;
    views.reserve(layers.size());
    for (const auto& l : layers) {
        views.push_back(to_view(l));
    }
    std::stable_sort(views.begin(), views.end(), [](const auto& a, const auto& b) { return a.z_order < b.z_order; });

    scene::EvaluatedCompositionState roi_state;
    roi_state.width = width;
    roi_state.height = height;
    roi_state.layers = layers;
    const TileGrid tile_grid = build_tile_grid(roi_state, 256);
    const bool use_tiles = tile_grid.roi.width > 0 &&
        (tile_grid.tiles.size() > 1 ||
         tile_grid.roi.x != 0 ||
         tile_grid.roi.y != 0 ||
         tile_grid.roi.width != width ||
         tile_grid.roi.height != height);

    const std::vector<renderer2d::RectI> render_tiles = use_tiles
        ? tile_grid.tiles
        : std::vector<renderer2d::RectI>{renderer2d::RectI{0, 0, static_cast<int>(width), static_cast<int>(height)}};

    for (const auto& tile : render_tiles) {
        const renderer2d::RectI render_region = use_tiles ? inflate_rect(tile, 32, width, height) : tile;
        std::map<std::size_t, std::shared_ptr<renderer2d::SurfaceRGBA>> rendered_cache;
        for (const auto& layer : views) {
            if (!layer.visible || layer.kind == RenderLayerKind::Camera) {
                continue;
            }

            auto layer_surface = std::make_shared<renderer2d::SurfaceRGBA>(render_region.width, render_region.height);
            layer_surface->clear(renderer2d::Color::transparent());
            render_layer_to_surface(*layer_surface, layer, width, height, render_region, plan, task);

            if (layer.track_matte_type != TrackMatteType::None && layer.track_matte_layer_index.has_value()) {
                auto it = rendered_cache.find(*layer.track_matte_layer_index);
                if (it != rendered_cache.end()) {
                    apply_track_matte(*layer_surface, *it->second, layer.track_matte_type);
                }
            }

            rendered_cache[layer.layer_index] = layer_surface;
            composite_surface_at(*frame.surface, *layer_surface, render_region.x, render_region.y, tile);
        }
    }

    return frame;
}

} // namespace

RasterizedFrame2D render_evaluated_composition_2d(
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task) {
    return render_composition_recursive(state.layers, state.width, state.height, plan, task);
}

RasterizedFrame2D render_evaluated_composition_2d(
    const timeline::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task) {
    
    (void)state;
    (void)plan;
    (void)task;

    RasterizedFrame2D frame;
    frame.width = state.width;
    frame.height = state.height;
    frame.surface.emplace(static_cast<uint32_t>(state.width), static_cast<uint32_t>(state.height));
    frame.surface->clear(renderer2d::Color::transparent());
    return frame; 
}

} // namespace tachyon
