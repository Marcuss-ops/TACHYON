#include "tachyon/renderer2d/framebuffer.h"
#include "tachyon/renderer2d/color_transfer.h"

#include <algorithm>
#include <string>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace tachyon {
namespace renderer2d {

SurfaceRGBA::SurfaceRGBA(uint32_t width, uint32_t height)
    : m_width(width),
      m_height(height),
      m_pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), pack(Color::transparent())),
      m_clip_rect{0, 0, static_cast<int>(width), static_cast<int>(height)} {
}

uint32_t SurfaceRGBA::pack(Color color) {
    return (static_cast<uint32_t>(color.a) << 24U) |
           (static_cast<uint32_t>(color.b) << 16U) |
           (static_cast<uint32_t>(color.g) << 8U) |
           (static_cast<uint32_t>(color.r));
}

Color SurfaceRGBA::unpack(uint32_t packed) {
    return Color{
        static_cast<uint8_t>(packed & 0xFFU),
        static_cast<uint8_t>((packed >> 8U) & 0xFFU),
        static_cast<uint8_t>((packed >> 16U) & 0xFFU),
        static_cast<uint8_t>((packed >> 24U) & 0xFFU)
    };
}

bool SurfaceRGBA::in_bounds(uint32_t x, uint32_t y) const {
    return x < m_width && y < m_height;
}

bool SurfaceRGBA::in_clip(uint32_t x, uint32_t y) const {
    return static_cast<int>(x) >= m_clip_rect.x &&
           static_cast<int>(y) >= m_clip_rect.y &&
           static_cast<int>(x) < (m_clip_rect.x + m_clip_rect.width) &&
           static_cast<int>(y) < (m_clip_rect.y + m_clip_rect.height);
}

RectI SurfaceRGBA::intersect_rects(const RectI& a, const RectI& b) const {
    const int x0 = std::max(a.x, b.x);
    const int y0 = std::max(a.y, b.y);
    const int x1 = std::min(a.x + a.width, b.x + b.width);
    const int y1 = std::min(a.y + a.height, b.y + b.height);

    if (x1 <= x0 || y1 <= y0) {
        return RectI{0, 0, 0, 0};
    }

    return RectI{x0, y0, x1 - x0, y1 - y0};
}

void SurfaceRGBA::clear(Color color) {
    std::fill(m_pixels.begin(), m_pixels.end(), pack(color));
}

bool SurfaceRGBA::set_clip_rect(const RectI& rect) {
    const RectI surface_bounds{0, 0, static_cast<int>(m_width), static_cast<int>(m_height)};
    m_clip_rect = intersect_rects(surface_bounds, rect);
    return m_clip_rect.width > 0 && m_clip_rect.height > 0;
}

void SurfaceRGBA::reset_clip_rect() {
    m_clip_rect = RectI{0, 0, static_cast<int>(m_width), static_cast<int>(m_height)};
}

RectI SurfaceRGBA::clip_rect() const {
    return m_clip_rect;
}

bool SurfaceRGBA::set_pixel(uint32_t x, uint32_t y, Color color) {
    if (!in_bounds(x, y) || !in_clip(x, y)) {
        return false;
    }

    m_pixels[static_cast<std::size_t>(y) * m_width + x] = pack(color);
    return true;
}

Color SurfaceRGBA::blend_src_over_premultiplied(Color src_straight, Color dst_straight) {
    return detail::composite_src_over_linear(src_straight, dst_straight);
}

bool SurfaceRGBA::blend_pixel(uint32_t x, uint32_t y, Color color) {
    if (!in_bounds(x, y) || !in_clip(x, y)) {
        return false;
    }

    const std::size_t index = static_cast<std::size_t>(y) * m_width + x;
    const Color dest = unpack(m_pixels[index]);
    m_pixels[index] = pack(blend_src_over_premultiplied(color, dest));
    return true;
}

std::optional<Color> SurfaceRGBA::try_get_pixel(uint32_t x, uint32_t y) const {
    if (!in_bounds(x, y)) {
        return std::nullopt;
    }

    return unpack(m_pixels[static_cast<std::size_t>(y) * m_width + x]);
}

Color SurfaceRGBA::get_pixel(uint32_t x, uint32_t y) const {
    const auto pixel = try_get_pixel(x, y);
    return pixel.has_value() ? *pixel : Color::transparent();
}

void SurfaceRGBA::fill_rect(const RectI& rect, Color color, bool blend) {
    if (rect.width <= 0 || rect.height <= 0) {
        return;
    }

    const RectI bounded = intersect_rects(rect, m_clip_rect);
    const RectI surface_bounds{0, 0, static_cast<int>(m_width), static_cast<int>(m_height)};
    const RectI draw_rect = intersect_rects(bounded, surface_bounds);
    if (draw_rect.width <= 0 || draw_rect.height <= 0) {
        return;
    }

    for (int y = draw_rect.y; y < draw_rect.y + draw_rect.height; ++y) {
        for (int x = draw_rect.x; x < draw_rect.x + draw_rect.width; ++x) {
            if (blend) {
                blend_pixel(static_cast<uint32_t>(x), static_cast<uint32_t>(y), color);
            } else {
                set_pixel(static_cast<uint32_t>(x), static_cast<uint32_t>(y), color);
            }
        }
    }
}

bool SurfaceRGBA::save_png(const std::filesystem::path& path) const {
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }

    std::string path_str = path.string();
    return stbi_write_png(path_str.c_str(),
                          static_cast<int>(m_width),
                          static_cast<int>(m_height),
                          4,
                          m_pixels.data(),
                          static_cast<int>(m_width * 4U)) != 0;
}

} // namespace renderer2d
} // namespace tachyon
