#include "tachyon/renderer2d/evaluated_composition_renderer.h"

#include "tachyon/renderer2d/effect_host.h"
#include "tachyon/renderer2d/path_rasterizer.h"
#include "tachyon/text/font.h"
#include "tachyon/text/layout.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace tachyon {
namespace {

enum class RenderLayerKind {
    Solid,
    Shape,
    Mask,
    Text,
    Image,
    Camera,
    Unknown
};

struct RenderLayerView {
    RenderLayerKind kind{RenderLayerKind::Unknown};
    bool visible{false};
    int z_order{0};
    float opacity{1.0f};
    math::Vector2 position{math::Vector2::zero()};
    math::Vector2 scale{math::Vector2::one()};
    std::string id;
    std::string label;
};

renderer2d::Color premultiply_color(renderer2d::Color color) {
    color.r = static_cast<std::uint8_t>((static_cast<std::uint32_t>(color.r) * color.a + 127U) / 255U);
    color.g = static_cast<std::uint8_t>((static_cast<std::uint32_t>(color.g) * color.a + 127U) / 255U);
    color.b = static_cast<std::uint8_t>((static_cast<std::uint32_t>(color.b) * color.a + 127U) / 255U);
    return color;
}

renderer2d::Color color_with_opacity(renderer2d::Color base, float opacity) {
    const float clamped = std::clamp(opacity, 0.0f, 1.0f);
    base.a = static_cast<std::uint8_t>(std::round(255.0f * clamped));
    return premultiply_color(base);
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

renderer2d::RectI layer_bounds(const RenderLayerView& layer, std::int64_t composition_width, std::int64_t composition_height) {
    const int base_w = layer.kind == RenderLayerKind::Text ? std::max(64, static_cast<int>(composition_width / 6)) : std::max(64, static_cast<int>(composition_width / 4));
    const int base_h = layer.kind == RenderLayerKind::Text ? std::max(32, static_cast<int>(composition_height / 10)) : std::max(32, static_cast<int>(composition_height / 8));
    const int width = std::max(1, static_cast<int>(std::round(static_cast<float>(base_w) * layer.scale.x)));
    const int height = std::max(1, static_cast<int>(std::round(static_cast<float>(base_h) * layer.scale.y)));
    return renderer2d::RectI{
        static_cast<int>(std::round(layer.position.x)),
        static_cast<int>(std::round(layer.position.y)),
        width,
        height
    };
}

RenderLayerView to_view(const scene::EvaluatedLayerState& layer) {
    RenderLayerView view;
    view.visible = layer.visible;
    view.z_order = static_cast<int>(layer.layer_index);
    view.opacity = static_cast<float>(layer.opacity);
    view.position = layer.position;
    view.scale = layer.scale;
    view.id = layer.id;
    view.label = layer.name.empty() ? layer.id : layer.name;

    if (layer.type == "solid") view.kind = RenderLayerKind::Solid;
    else if (layer.type == "shape") view.kind = RenderLayerKind::Shape;
    else if (layer.type == "mask") view.kind = RenderLayerKind::Mask;
    else if (layer.type == "text") view.kind = RenderLayerKind::Text;
    else if (layer.type == "image") view.kind = RenderLayerKind::Image;
    else if (layer.is_camera) view.kind = RenderLayerKind::Camera;
    else view.kind = RenderLayerKind::Unknown;

    return view;
}

RenderLayerView to_view(const timeline::EvaluatedLayerState& layer) {
    RenderLayerView view;
    view.visible = layer.visible;
    view.z_order = layer.z_order;
    view.opacity = layer.opacity;
    view.position = layer.transform2.position;
    view.scale = layer.transform2.scale;
    view.id = layer.id;
    view.label = layer.text.has_value() ? layer.text->text : layer.id;

    switch (layer.type) {
        case timeline::LayerType::Solid: view.kind = RenderLayerKind::Solid; break;
        case timeline::LayerType::Shape: view.kind = RenderLayerKind::Shape; break;
        case timeline::LayerType::Mask: view.kind = RenderLayerKind::Mask; break;
        case timeline::LayerType::Text: view.kind = RenderLayerKind::Text; break;
        case timeline::LayerType::Image: view.kind = RenderLayerKind::Image; break;
        case timeline::LayerType::Camera: view.kind = RenderLayerKind::Camera; break;
        default: view.kind = RenderLayerKind::Unknown; break;
    }

    return view;
}

std::filesystem::path find_default_font_path() {
    const std::filesystem::path candidates[] = {
        "Tachyon/tests/fixtures/fonts/minimal_5x7.bdf",
        "tests/fixtures/fonts/minimal_5x7.bdf"
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
    }
    return host;
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

void render_text_layer(
    renderer2d::SurfaceRGBA& layer_surface,
    const RenderLayerView& layer,
    const renderer2d::RectI& clip,
    std::int64_t composition_width,
    std::int64_t composition_height) {

    const text::BitmapFont* font = default_font();
    if (font == nullptr) {
        return;
    }

    text::TextStyle style;
    style.pixel_size = 14;
    style.fill_color = renderer2d::Color::white();
    style.fill_color.a = static_cast<std::uint8_t>(std::round(255.0f * std::clamp(layer.opacity, 0.0f, 1.0f)));

    text::TextBox box;
    box.width = static_cast<std::uint32_t>(std::max<std::int64_t>(64, composition_width / 3));
    box.height = static_cast<std::uint32_t>(std::max<std::int64_t>(32, composition_height / 5));
    box.multiline = true;

    const text::TextRasterSurface text_surface = text::rasterize_text_rgba(*font, layer.label, style, box, text::TextAlignment::Left);
    const int offset_x = static_cast<int>(std::round(layer.position.x));
    const int offset_y = static_cast<int>(std::round(layer.position.y));

    const int start_x = std::max(clip.x, offset_x);
    const int start_y = std::max(clip.y, offset_y);
    const int end_x = std::min(clip.x + clip.width, offset_x + static_cast<int>(text_surface.width()));
    const int end_y = std::min(clip.y + clip.height, offset_y + static_cast<int>(text_surface.height()));

    for (int y = start_y; y < end_y; ++y) {
        for (int x = start_x; x < end_x; ++x) {
            const auto pixel = text_surface.get_pixel(static_cast<std::uint32_t>(x - offset_x), static_cast<std::uint32_t>(y - offset_y));
            if (pixel.a == 0U) {
                continue;
            }
            layer_surface.blend_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), premultiply_color(pixel));
        }
    }
}

void render_layer(
    renderer2d::SurfaceRGBA& destination,
    const RenderLayerView& layer,
    std::int64_t composition_width,
    std::int64_t composition_height,
    renderer2d::RectI& active_clip) {

    if (!layer.visible || layer.kind == RenderLayerKind::Camera) {
        return;
    }

    if (layer.kind == RenderLayerKind::Mask) {
        active_clip = intersect_rects(active_clip, layer_bounds(layer, composition_width, composition_height));
        return;
    }

    renderer2d::SurfaceRGBA layer_surface(destination.width(), destination.height());
    layer_surface.clear(renderer2d::Color::transparent());
    layer_surface.set_clip_rect(active_clip);

    const renderer2d::RectI bounds = layer_bounds(layer, composition_width, composition_height);

    switch (layer.kind) {
        case RenderLayerKind::Solid: {
            const renderer2d::Color color = color_with_opacity(renderer2d::Color{64, 128, 255, 255}, layer.opacity);
            layer_surface.fill_rect(bounds, color);
            break;
        }
        case RenderLayerKind::Shape: {
            renderer2d::PathGeometry path;
            path.commands = {
                {renderer2d::PathVerb::MoveTo, {static_cast<float>(bounds.x), static_cast<float>(bounds.y)}},
                {renderer2d::PathVerb::LineTo, {static_cast<float>(bounds.x + bounds.width), static_cast<float>(bounds.y)}},
                {renderer2d::PathVerb::LineTo, {static_cast<float>(bounds.x + bounds.width), static_cast<float>(bounds.y + bounds.height)}},
                {renderer2d::PathVerb::LineTo, {static_cast<float>(bounds.x), static_cast<float>(bounds.y + bounds.height)}},
                {renderer2d::PathVerb::Close}
            };

            renderer2d::FillPathStyle fill_style;
            fill_style.fill_color = color_with_opacity(renderer2d::Color{64, 255, 160, 255}, layer.opacity);
            fill_style.opacity = 1.0f;
            renderer2d::PathRasterizer::fill(layer_surface, path, fill_style);

            renderer2d::StrokePathStyle stroke_style;
            stroke_style.stroke_color = color_with_opacity(renderer2d::Color{20, 40, 30, 255}, layer.opacity);
            stroke_style.stroke_width = 1.0f;
            stroke_style.opacity = 1.0f;
            renderer2d::PathRasterizer::stroke(layer_surface, path, stroke_style);
            break;
        }
        case RenderLayerKind::Text:
            render_text_layer(layer_surface, layer, active_clip, composition_width, composition_height);
            break;
        case RenderLayerKind::Image: {
            const renderer2d::Color color = color_with_opacity(renderer2d::Color{255, 180, 64, 255}, layer.opacity);
            layer_surface.fill_rect(bounds, color);
            break;
        }
        case RenderLayerKind::Unknown:
        default: {
            const renderer2d::Color color = color_with_opacity(renderer2d::Color{160, 160, 160, 255}, layer.opacity);
            layer_surface.fill_rect(bounds, color);
            break;
        }
        case RenderLayerKind::Camera:
        case RenderLayerKind::Mask:
            break;
    }

    renderer2d::EffectParams effect_params;
    effect_params.scalars["blur_radius"] = 0.0f;
    layer_surface = scene_effect_host().apply("identity", layer_surface, effect_params);
    composite_surface(destination, layer_surface, active_clip);
}

std::vector<renderer2d::DrawCommand2D> build_debug_draw_commands(const timeline::EvaluatedCompositionState& state) {
    std::vector<renderer2d::DrawCommand2D> commands;
    commands.reserve(state.layers.size() + 1);

    renderer2d::DrawCommand2D clear;
    clear.kind = renderer2d::DrawCommandKind::Clear;
    clear.clear.emplace(renderer2d::ClearCommand{renderer2d::Color::transparent()});
    commands.push_back(clear);

    for (const auto& layer : state.layers) {
        renderer2d::DrawCommand2D command;
        command.kind = renderer2d::DrawCommandKind::SolidRect;
        command.z_order = layer.z_order;
        command.solid_rect.emplace(renderer2d::SolidRectCommand{
            renderer2d::RectI{
                static_cast<int>(std::round(layer.transform2.position.x)),
                static_cast<int>(std::round(layer.transform2.position.y)),
                std::max(1, static_cast<int>(std::round(100.0f * layer.transform2.scale.x))),
                std::max(1, static_cast<int>(std::round(100.0f * layer.transform2.scale.y)))
            },
            color_with_opacity(renderer2d::Color{64, 128, 255, 255}, layer.opacity),
            layer.opacity
        });
        commands.push_back(command);
    }

    return commands;
}

template <typename LayerRange>
RasterizedFrame2D render_composition(
    const LayerRange& layers,
    std::int64_t width,
    std::int64_t height,
    const RenderPlan& plan,
    const FrameRenderTask& task) {

    RasterizedFrame2D frame;
    frame.frame_number = task.frame_number;
    frame.width = width;
    frame.height = height;
    frame.layer_count = layers.size();
    frame.estimated_draw_ops = layers.size();
    frame.backend_name = "cpu-2d-evaluated-composition";
    frame.cache_key = task.cache_key.value;
    frame.note = "Frame rasterized from evaluated composition " + plan.composition.id;
    frame.surface.emplace(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
    if (!frame.surface.has_value()) {
        return frame;
    }

    frame.surface->clear(renderer2d::Color::transparent());
    renderer2d::RectI active_clip = full_rect(width, height);

    std::vector<RenderLayerView> ordered;
    ordered.reserve(layers.size());
    for (const auto& layer : layers) {
        ordered.push_back(to_view(layer));
    }

    std::stable_sort(ordered.begin(), ordered.end(), [](const RenderLayerView& a, const RenderLayerView& b) {
        return a.z_order < b.z_order;
    });

    for (const auto& layer : ordered) {
        render_layer(*frame.surface, layer, width, height, active_clip);
    }

    frame.surface->reset_clip_rect();
    return frame;
}

} // namespace

std::vector<renderer2d::DrawCommand2D> build_draw_commands_from_evaluated_state(
    const timeline::EvaluatedCompositionState& state) {
    return build_debug_draw_commands(state);
}

RasterizedFrame2D render_evaluated_composition_2d(
    const timeline::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task) {

    return render_composition(state.layers, state.width, state.height, plan, task);
}

RasterizedFrame2D render_evaluated_composition_2d(
    const scene::EvaluatedCompositionState& state,
    const RenderPlan& plan,
    const FrameRenderTask& task) {

    return render_composition(state.layers, state.width, state.height, plan, task);
}

} // namespace tachyon
