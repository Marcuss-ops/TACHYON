#pragma once

#include "tachyon/renderer2d/framebuffer.h"
#include <algorithm>

namespace tachyon {
namespace renderer2d {

/**
 * Utility for Premultiplied Alpha Blending (SrcOver).
 */
struct Blender {
    /**
     * Blends a source pixel onto a destination pixel using Premultiplied Alpha.
     * Formula: Result = Src + Dest * (1 - Src.Alpha)
     */
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

/**
 * Basic rectangle primitive.
 */
struct RectPrimitive {
    int x{0}, y{0};
    int width{0}, height{0};
    Color color;
};

/**
 * Basic line primitive.
 */
struct LinePrimitive {
    int x0{0}, y0{0};
    int x1{0}, y1{0};
    Color color;
};

} // namespace renderer2d
} // namespace tachyon
