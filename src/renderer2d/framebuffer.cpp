#include "tachyon/renderer2d/framebuffer.h"
#include "tachyon/renderer2d/color_transfer.h"

#include "stb_image_write.h"

#include <algorithm>

namespace tachyon {
namespace renderer2d {

namespace {

Color blend_premultiplied(Color src, Color dst) {
    return detail::composite_src_over_linear(src, dst);
}

} // namespace

SurfaceRGBA::SurfaceRGBA(uint32_t width, uint32_t height)
    : m_width(width),
      m_height(height),
      m_pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), pack(Color::transparent())),
      m_clip_rect{0, 0, static_cast<int>(width), static_cast<int>(height)} {
}

void SurfaceRGBA::clear(Color color) {
    std::fill(m_pixels.begin(), m_pixels.end(), pack(color));
}

void SurfaceRGBA::clear_depth(float depth) {
    if (m_depth_buffer.size() != m_pixels.size()) {
        m_depth_buffer.assign(m_pixels.size(), depth);
    } else {
        std::fill(m_depth_buffer.begin(), m_depth_buffer.end(), depth);
    }
}

bool SurfaceRGBA::set_clip_rect(const RectI& rect) {
    const RectI bounds{0, 0, static_cast<int>(m_width), static_cast<int>(m_height)};
    m_clip_rect = intersect_rects(bounds, rect);
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

    const std::size_t index = static_cast<std::size_t>(y) * m_width + x;
    const Color dst = unpack(m_pixels[index]);
    m_pixels[index] = pack(blend_src_over_premultiplied(color, dst));
    return true;
}

bool SurfaceRGBA::test_and_write_depth(uint32_t x, uint32_t y, float inv_z) {
    if (!in_bounds(x, y) || !in_clip(x, y)) {
        return false;
    }

    if (m_depth_buffer.empty()) {
        clear_depth(0.0f); // Auto-initialize depth buffer if used
    }

    const std::size_t index = static_cast<std::size_t>(y) * m_width + x;
    if (inv_z >= m_depth_buffer[index]) {
        m_depth_buffer[index] = inv_z;
        return true;
    }
    return false;
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

float SurfaceRGBA::get_depth(uint32_t x, uint32_t y) const {
    if (!in_bounds(x, y) || m_depth_buffer.empty()) {
        return 0.0f;
    }
    return m_depth_buffer[static_cast<std::size_t>(y) * m_width + x];
}

void SurfaceRGBA::fill_rect(const RectI& rect, Color color, bool blend) {
    const RectI clipped = intersect_rects(m_clip_rect, rect);
    if (clipped.width <= 0 || clipped.height <= 0) {
        return;
    }

    for (int y = clipped.y; y < clipped.y + clipped.height; ++y) {
        for (int x = clipped.x; x < clipped.x + clipped.width; ++x) {
            if (blend) {
                blend_pixel(static_cast<uint32_t>(x), static_cast<uint32_t>(y), color);
            } else {
                set_pixel(static_cast<uint32_t>(x), static_cast<uint32_t>(y), color);
            }
        }
    }
}

bool SurfaceRGBA::save_png(const std::filesystem::path& path) const {
    if (m_width == 0 || m_height == 0) {
        return false;
    }

    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }

    std::vector<std::uint8_t> rgba_bytes;
    rgba_bytes.reserve(m_pixels.size() * 4U);
    for (const std::uint32_t packed : m_pixels) {
        const Color color = unpack(packed);
        rgba_bytes.push_back(color.r);
        rgba_bytes.push_back(color.g);
        rgba_bytes.push_back(color.b);
        rgba_bytes.push_back(color.a);
    }

    return stbi_write_png(path.string().c_str(),
                          static_cast<int>(m_width),
                          static_cast<int>(m_height),
                          4,
                          rgba_bytes.data(),
                          static_cast<int>(m_width * 4U)) != 0;
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

uint32_t SurfaceRGBA::pack(Color color) {
    return static_cast<uint32_t>(color.r) |
           (static_cast<uint32_t>(color.g) << 8U) |
           (static_cast<uint32_t>(color.b) << 16U) |
           (static_cast<uint32_t>(color.a) << 24U);
}

Color SurfaceRGBA::unpack(uint32_t packed) {
    return Color{
        static_cast<std::uint8_t>(packed & 0xFFU),
        static_cast<std::uint8_t>((packed >> 8U) & 0xFFU),
        static_cast<std::uint8_t>((packed >> 16U) & 0xFFU),
        static_cast<std::uint8_t>((packed >> 24U) & 0xFFU)
    };
}

Color SurfaceRGBA::blend_src_over_premultiplied(Color src_straight, Color dst_straight) {
    return blend_premultiplied(src_straight, dst_straight);
}

RectI SurfaceRGBA::intersect_rects(const RectI& a, const RectI& b) const {
    const int x0 = std::max(a.x, b.x);
    const int y0 = std::max(a.y, b.y);
    const int x1 = std::min(a.x + a.width, b.x + b.width);
    const int y1 = std::min(a.y + a.height, b.y + b.height);

    return RectI{x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)};
}

} // namespace renderer2d
} // namespace tachyon
