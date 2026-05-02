#include "tachyon/renderer2d/raster/draw_primitives.h"
#include <algorithm>

namespace tachyon::renderer2d {

void draw_disk(SurfaceRGBA& surface, int cx, int cy, int radius, Color color) {
    if (radius <= 0) {
        if (cx >= 0 && cy >= 0 && static_cast<std::uint32_t>(cx) < surface.width() && static_cast<std::uint32_t>(cy) < surface.height())
            surface.blend_pixel(static_cast<std::uint32_t>(cx), static_cast<std::uint32_t>(cy), color);
        return;
    }
    const int r2 = radius * radius;
    const int min_x = std::max(0, cx - radius), max_x = std::min(static_cast<int>(surface.width()) - 1, cx + radius);
    const int min_y = std::max(0, cy - radius), max_y = std::min(static_cast<int>(surface.height()) - 1, cy + radius);
    for (int y = min_y; y <= max_y; ++y)
        for (int x = min_x; x <= max_x; ++x)
            if (((x - cx) * (x - cx) + (y - cy) * (y - cy)) <= r2)
                surface.blend_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), color);
}

} // namespace tachyon::renderer2d
