#pragma once

#include "tachyon/renderer2d/draw_command.h"
#include "tachyon/renderer2d/color_transfer.h"
#include "tachyon/renderer2d/framebuffer.h"
#include <algorithm>

namespace tachyon {
namespace renderer2d {

struct Blender {
    static Color composite_premultiplied(Color src, Color dest) {
        return detail::composite_src_over_linear(src, dest);
    }
};

inline Color blend_mode_color(Color src, Color dest, BlendMode mode) {
    if (mode == BlendMode::Normal) {
        return detail::composite_src_over_linear(src, dest);
    }

    detail::LinearPremultipliedPixel src_linear = detail::to_premultiplied(src, detail::TransferCurve::Srgb);
    detail::LinearPremultipliedPixel dst_linear = detail::to_premultiplied(dest, detail::TransferCurve::Srgb);

    detail::LinearPremultipliedPixel out;

    switch (mode) {
    case BlendMode::Normal:
        break; // Handled above
    case BlendMode::Additive:
        out.r = std::min(1.0f, src_linear.r + dst_linear.r);
        out.g = std::min(1.0f, src_linear.g + dst_linear.g);
        out.b = std::min(1.0f, src_linear.b + dst_linear.b);
        out.a = std::min(1.0f, src_linear.a + dst_linear.a);
        break;
    case BlendMode::Multiply:
        out.r = src_linear.r * dst_linear.r + src_linear.r * (1.0f - dst_linear.a) + dst_linear.r * (1.0f - src_linear.a);
        out.g = src_linear.g * dst_linear.g + src_linear.g * (1.0f - dst_linear.a) + dst_linear.g * (1.0f - src_linear.a);
        out.b = src_linear.b * dst_linear.b + src_linear.b * (1.0f - dst_linear.a) + dst_linear.b * (1.0f - src_linear.a);
        out.a = src_linear.a + dst_linear.a - src_linear.a * dst_linear.a;
        break;
    case BlendMode::Screen:
        out.r = src_linear.r + dst_linear.r - src_linear.r * dst_linear.r;
        out.g = src_linear.g + dst_linear.g - src_linear.g * dst_linear.g;
        out.b = src_linear.b + dst_linear.b - src_linear.b * dst_linear.b;
        out.a = src_linear.a + dst_linear.a - src_linear.a * dst_linear.a;
        break;
    }

    return detail::from_premultiplied(out, detail::TransferCurve::Srgb);
}

struct ClearPrimitive {
    Color color{Color::transparent()};
};

struct RectPrimitive {
    int x{0}, y{0};
    int width{0}, height{0};
    Color color;
};

struct LinePrimitive {
    int x0{0}, y0{0};
    int x1{0}, y1{0};
    Color color;
};

struct TexturedVertex2D {
    float x{0.0F};
    float y{0.0F};
    float u{0.0F};
    float v{0.0F};
};

struct TexturedQuadPrimitive {
    int x{0};
    int y{0};
    int width{0};
    int height{0};
    const SurfaceRGBA* texture{nullptr};
    Color tint{Color::white()};
    bool use_custom_vertices{false};
    TexturedVertex2D vertices[4]{};

    static TexturedQuadPrimitive axis_aligned(
        int x,
        int y,
        int width,
        int height,
        const SurfaceRGBA* texture,
        Color tint = Color::white()) {
        TexturedQuadPrimitive quad;
        quad.x = x;
        quad.y = y;
        quad.width = width;
        quad.height = height;
        quad.texture = texture;
        quad.tint = tint;
        quad.vertices[0] = TexturedVertex2D{static_cast<float>(x), static_cast<float>(y), 0.0F, 0.0F};
        quad.vertices[1] = TexturedVertex2D{static_cast<float>(x + width), static_cast<float>(y), 1.0F, 0.0F};
        quad.vertices[2] = TexturedVertex2D{static_cast<float>(x + width), static_cast<float>(y + height), 1.0F, 1.0F};
        quad.vertices[3] = TexturedVertex2D{static_cast<float>(x), static_cast<float>(y + height), 0.0F, 1.0F};
        return quad;
    }

    static TexturedQuadPrimitive custom(
        const TexturedVertex2D& v0,
        const TexturedVertex2D& v1,
        const TexturedVertex2D& v2,
        const TexturedVertex2D& v3,
        const SurfaceRGBA* texture,
        Color tint = Color::white()) {
        TexturedQuadPrimitive quad;
        quad.texture = texture;
        quad.tint = tint;
        quad.use_custom_vertices = true;
        quad.vertices[0] = v0;
        quad.vertices[1] = v1;
        quad.vertices[2] = v2;
        quad.vertices[3] = v3;
        return quad;
    }
};

struct RasterizerCommand2D {
    enum class Kind {
        Clear,
        Rect,
        Line,
        TexturedQuad
    };

    Kind kind{Kind::Clear};
    bool has_clip_rect{false};
    RectI clip_rect{};
    ClearPrimitive clear{};
    RectPrimitive rect{};
    LinePrimitive line{};
    TexturedQuadPrimitive textured_quad{};

    static RasterizerCommand2D make_clear(Color color) {
        RasterizerCommand2D command;
        command.kind = Kind::Clear;
        command.clear = ClearPrimitive{color};
        return command;
    }

    static RasterizerCommand2D make_rect(const RectPrimitive& rect_primitive) {
        RasterizerCommand2D command;
        command.kind = Kind::Rect;
        command.rect = rect_primitive;
        return command;
    }

    static RasterizerCommand2D make_line(const LinePrimitive& line_primitive) {
        RasterizerCommand2D command;
        command.kind = Kind::Line;
        command.line = line_primitive;
        return command;
    }

    static RasterizerCommand2D make_textured_quad(const TexturedQuadPrimitive& textured_quad_primitive) {
        RasterizerCommand2D command;
        command.kind = Kind::TexturedQuad;
        command.textured_quad = textured_quad_primitive;
        return command;
    }

    RasterizerCommand2D& with_clip_rect(const RectI& clip_rect_value) {
        has_clip_rect = true;
        this->clip_rect = clip_rect_value;
        return *this;
    }
};

} // namespace renderer2d
} // namespace tachyon
