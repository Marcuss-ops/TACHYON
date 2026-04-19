#pragma once

#include "tachyon/renderer2d/framebuffer.h"
#include <algorithm>

namespace tachyon {
namespace renderer2d {

struct Blender {
    static Color composite_premultiplied(Color src, Color dest) {
        if (src.a == 255) return src;
        if (src.a == 0) return dest;

        uint32_t inv_a = 255 - src.a;

        return Color{
            static_cast<uint8_t>(std::clamp<uint32_t>(src.r + (dest.r * inv_a + 127) / 255, 0, 255)),
            static_cast<uint8_t>(std::clamp<uint32_t>(src.g + (dest.g * inv_a + 127) / 255, 0, 255)),
            static_cast<uint8_t>(std::clamp<uint32_t>(src.b + (dest.b * inv_a + 127) / 255, 0, 255)),
            static_cast<uint8_t>(std::clamp<uint32_t>(src.a + (dest.a * inv_a + 127) / 255, 0, 255))
        };
    }
};

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

struct TexturedQuadPrimitive {
    int x{0};
    int y{0};
    int width{0};
    int height{0};
    const SurfaceRGBA* texture{nullptr};
    Color tint{Color::white()};
};

struct DrawCommand2D {
    enum class Kind {
        Clear,
        Rect,
        Line,
        TexturedQuad
    };

    Kind kind{Kind::Clear};
    ClearPrimitive clear{};
    RectPrimitive rect{};
    LinePrimitive line{};
    TexturedQuadPrimitive textured_quad{};

    static DrawCommand2D make_clear(Color color) {
        DrawCommand2D command;
        command.kind = Kind::Clear;
        command.clear = ClearPrimitive{color};
        return command;
    }

    static DrawCommand2D make_rect(const RectPrimitive& rect_primitive) {
        DrawCommand2D command;
        command.kind = Kind::Rect;
        command.rect = rect_primitive;
        return command;
    }

    static DrawCommand2D make_line(const LinePrimitive& line_primitive) {
        DrawCommand2D command;
        command.kind = Kind::Line;
        command.line = line_primitive;
        return command;
    }

    static DrawCommand2D make_textured_quad(const TexturedQuadPrimitive& textured_quad_primitive) {
        DrawCommand2D command;
        command.kind = Kind::TexturedQuad;
        command.textured_quad = textured_quad_primitive;
        return command;
    }
};

} // namespace renderer2d
} // namespace tachyon
