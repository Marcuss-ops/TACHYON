#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/color/color_transfer.h"

#include <algorithm>
#include <string>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace tachyon {
namespace renderer2d {

SurfaceRGBA::SurfaceRGBA(uint32_t width, uint32_t height)
    : m_width(width),
      m_height(height),
      m_pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U, 0.0f),
      m_clip_rect{0, 0, static_cast<int>(width), static_cast<int>(height)} {
}

void SurfaceRGBA::reset(uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;
    m_pixels.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U, 0.0f);
    m_depth_buffer.clear();
    reset_clip_rect();
}

void SurfaceRGBA::clear(Color color) {
    for (std::size_t i = 0; i < m_pixels.size(); i += 4U) {
        m_pixels[i + 0] = color.r;
        m_pixels[i + 1] = color.g;
        m_pixels[i + 2] = color.b;
        m_pixels[i + 3] = color.a;
    }
}

void SurfaceRGBA::clear_depth(float depth) {
    const std::size_t pixel_count = m_width * m_height;
    if (m_depth_buffer.size() != pixel_count) {
        m_depth_buffer.assign(pixel_count, depth);
    } else {
        std::fill(m_depth_buffer.begin(), m_depth_buffer.end(), depth);
    }
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

    const std::size_t index = (static_cast<std::size_t>(y) * m_width + x) * 4U;
    m_pixels[index + 0] = color.r;
    m_pixels[index + 1] = color.g;
    m_pixels[index + 2] = color.b;
    m_pixels[index + 3] = color.a;
    return true;
}

bool SurfaceRGBA::set_pixel_with_depth(uint32_t x, uint32_t y, Color color, float inv_z) {
    if (test_and_write_depth(x, y, inv_z)) {
        return set_pixel(x, y, color);
    }
    return false;
}

bool SurfaceRGBA::blend_pixel(uint32_t x, uint32_t y, Color color) {
    if (!in_bounds(x, y) || !in_clip(x, y)) {
        return false;
    }

    const std::size_t index = (static_cast<std::size_t>(y) * m_width + x) * 4U;
    const Color dest{m_pixels[index + 0], m_pixels[index + 1], m_pixels[index + 2], m_pixels[index + 3]};
    const Color blended = detail::composite_src_over_linear(color, dest);
    
    m_pixels[index + 0] = blended.r;
    m_pixels[index + 1] = blended.g;
    m_pixels[index + 2] = blended.b;
    m_pixels[index + 3] = blended.a;
    return true;
}

bool SurfaceRGBA::test_and_write_depth(uint32_t x, uint32_t y, float inv_z) {
    if (!in_bounds(x, y) || !in_clip(x, y)) {
        return false;
    }

    if (m_depth_buffer.empty()) {
        clear_depth(0.0f);
    }

    const std::size_t index = static_cast<std::size_t>(y) * m_width + x;
    if (inv_z >= m_depth_buffer[index]) {
        m_depth_buffer[index] = inv_z;
        return true;
    }
    return false;
}

Color SurfaceRGBA::get_pixel(uint32_t x, uint32_t y) const {
    if (!in_bounds(x, y)) {
        return Color::transparent();
    }

    const std::size_t index = (static_cast<std::size_t>(y) * m_width + x) * 4U;
    return Color{m_pixels[index + 0], m_pixels[index + 1], m_pixels[index + 2], m_pixels[index + 3]};
}

std::optional<Color> SurfaceRGBA::try_get_pixel(uint32_t x, uint32_t y) const {
    if (!in_bounds(x, y)) {
        return std::nullopt;
    }
    return get_pixel(x, y);
}

float SurfaceRGBA::get_depth(uint32_t x, uint32_t y) const {
    if (!in_bounds(x, y) || m_depth_buffer.empty()) {
        return 0.0f;
    }
    return m_depth_buffer[static_cast<std::size_t>(y) * m_width + x];
}

void SurfaceRGBA::fill_rect(const RectI& rect, Color color, bool blend) {
    const RectI bounded = intersect_rects(rect, m_clip_rect);
    if (bounded.width <= 0 || bounded.height <= 0) {
        return;
    }

    for (int y = bounded.y; y < bounded.y + bounded.height; ++y) {
        for (int x = bounded.x; x < bounded.x + bounded.width; ++x) {
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

    std::vector<uint8_t> byte_pixels;
    byte_pixels.reserve(m_width * m_height * 4U);
    
    for (std::size_t i = 0; i < m_pixels.size(); i += 4U) {
        Color linear{m_pixels[i + 0], m_pixels[i + 1], m_pixels[i + 2], m_pixels[i + 3]};
        Color srgb = detail::Linear_to_sRGB(linear);
        byte_pixels.push_back(static_cast<uint8_t>(std::clamp(srgb.r * 255.0f, 0.0f, 255.0f)));
        byte_pixels.push_back(static_cast<uint8_t>(std::clamp(srgb.g * 255.0f, 0.0f, 255.0f)));
        byte_pixels.push_back(static_cast<uint8_t>(std::clamp(srgb.b * 255.0f, 0.0f, 255.0f)));
        byte_pixels.push_back(static_cast<uint8_t>(std::clamp(srgb.a * 255.0f, 0.0f, 255.0f)));
    }

    std::string path_str = path.string();
    return stbi_write_png(path_str.c_str(),
                          static_cast<int>(m_width),
                          static_cast<int>(m_height),
                          4,
                          byte_pixels.data(),
                          static_cast<int>(m_width * 4U)) != 0;
}

} // namespace renderer2d
} // namespace tachyon
