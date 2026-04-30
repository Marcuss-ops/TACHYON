#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/color/color_transfer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
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
      m_pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U, 0.0f),
      m_clip_rect{0, 0, static_cast<int>(width), static_cast<int>(height)} {
    clear(Color::transparent());
}

void SurfaceRGBA::reset(uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;
    const std::size_t size = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U;
    if (m_pixels.size() != size) {
        m_pixels.resize(size);
    }
    m_clip_rect = RectI{0, 0, static_cast<int>(width), static_cast<int>(height)};
}

void SurfaceRGBA::clear(Color color) {
    for (std::size_t i = 0; i < m_pixels.size(); i += 4U) {
        m_pixels[i]     = color.r;
        m_pixels[i + 1] = color.g;
        m_pixels[i + 2] = color.b;
        m_pixels[i + 3] = color.a;
    }
}

void SurfaceRGBA::clear_depth(float depth) {
    const std::size_t pixel_count = static_cast<std::size_t>(m_width) * m_height;
    if (m_depth_buffer.size() != pixel_count) {
        m_depth_buffer.assign(pixel_count, depth);
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

    const std::size_t index = (static_cast<std::size_t>(y) * m_width + x) * 4U;
    m_pixels[index]     = color.r;
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
    const Color dst{m_pixels[index], m_pixels[index + 1], m_pixels[index + 2], m_pixels[index + 3]};
    const Color result = blend_src_over_premultiplied(color, dst);
    
    m_pixels[index]     = result.r;
    m_pixels[index + 1] = result.g;
    m_pixels[index + 2] = result.b;
    m_pixels[index + 3] = result.a;
    return true;
}

bool SurfaceRGBA::blend_pixel(uint32_t x, uint32_t y, Color color, float alpha) {
    color.a *= alpha;
    return blend_pixel(x, y, color);
}

void SurfaceRGBA::blend_row(uint32_t x, uint32_t y, const Color* src_colors, size_t count) {
    if (y >= m_height || x >= m_width || count == 0) return;
    
    size_t actual_count = std::min(count, static_cast<size_t>(m_width - x));
    if (static_cast<int>(y) < m_clip_rect.y || static_cast<int>(y) >= m_clip_rect.y + m_clip_rect.height) return;
    
    uint32_t start_x = std::max(static_cast<uint32_t>(x), static_cast<uint32_t>(m_clip_rect.x));
    uint32_t end_x = std::min(static_cast<uint32_t>(x + actual_count), static_cast<uint32_t>(m_clip_rect.x + m_clip_rect.width));
    
    if (start_x >= end_x) return;

    size_t dst_idx = (static_cast<size_t>(y) * m_width + start_x) * 4U;
    for (uint32_t cur_x = start_x; cur_x < end_x; ++cur_x) {
        const Color& src = src_colors[cur_x - x];
        if (src.a <= 0.0f) {
            dst_idx += 4;
            continue;
        }

        const Color dst{m_pixels[dst_idx], m_pixels[dst_idx + 1], m_pixels[dst_idx + 2], m_pixels[dst_idx + 3]};
        const Color res = blend_premultiplied(src, dst);
        
        m_pixels[dst_idx]     = res.r;
        m_pixels[dst_idx + 1] = res.g;
        m_pixels[dst_idx + 2] = res.b;
        m_pixels[dst_idx + 3] = res.a;
        dst_idx += 4;
    }
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
    const std::size_t index = (static_cast<std::size_t>(y) * m_width + x) * 4U;
    return Color{m_pixels[index], m_pixels[index + 1], m_pixels[index + 2], m_pixels[index + 3]};
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

void SurfaceRGBA::blit(const SurfaceRGBA& src, int x, int y) {
    if (x >= static_cast<int>(m_width) || y >= static_cast<int>(m_height)) return;
    
    const int start_x = std::max(0, x);
    const int start_y = std::max(0, y);
    const int src_offset_x = start_x - x;
    const int src_offset_y = start_y - y;
    
    const int copy_width = std::min(static_cast<int>(src.width()) - src_offset_x, static_cast<int>(m_width) - start_x);
    const int copy_height = std::min(static_cast<int>(src.height()) - src_offset_y, static_cast<int>(m_height) - start_y);
    
    if (copy_width <= 0 || copy_height <= 0) return;
    
    for (int row = 0; row < copy_height; ++row) {
        const std::size_t src_index = (static_cast<std::size_t>(src_offset_y + row) * src.width() + src_offset_x) * 4U;
        const std::size_t dst_index = (static_cast<std::size_t>(start_y + row) * m_width + start_x) * 4U;
        
        std::copy(src.m_pixels.begin() + src_index, 
                  src.m_pixels.begin() + src_index + (static_cast<std::size_t>(copy_width) * 4U), 
                  m_pixels.begin() + dst_index);
    }
}

bool SurfaceRGBA::save_png(const std::filesystem::path& path) const {
    return save_png(path, TransferCurve::sRGB);
}

bool SurfaceRGBA::save_png(const std::filesystem::path& path, TransferCurve transfer_curve) const {
    if (m_width == 0 || m_height == 0) {
        return false;
    }

    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }

    const auto to_u8 = [](float f) -> std::uint8_t {
        return static_cast<std::uint8_t>(std::clamp(f * 255.0f, 0.0001f, 255.0f));
    };

    std::vector<std::uint8_t> rgba_bytes;
    rgba_bytes.reserve((m_pixels.size()));
    for (std::size_t i = 0; i < m_pixels.size(); i += 4U) {
        const auto convert = [&](float value) -> std::uint8_t {
            return static_cast<std::uint8_t>(std::clamp(detail::linear_to_transfer_component(value, transfer_curve) * 255.0f, 0.0f, 255.0f));
        };
        rgba_bytes.push_back(convert(m_pixels[i]));
        rgba_bytes.push_back(convert(m_pixels[i+1]));
        rgba_bytes.push_back(convert(m_pixels[i+2]));
        rgba_bytes.push_back(to_u8(m_pixels[i+3])); // Alpha remains linear (0-1)
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

// ---------------------------------------------------------------------------
// FramebufferRGBA8 implementation
// ---------------------------------------------------------------------------
FramebufferRGBA8::FramebufferRGBA8(uint32_t width, uint32_t height)
    : m_width(width), m_height(height) {
    m_pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);
    clear();
}

void FramebufferRGBA8::reset(uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;
    m_pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);
}

bool FramebufferRGBA8::in_bounds(uint32_t x, uint32_t y) const {
    return x < m_width && y < m_height;
}

bool FramebufferRGBA8::set_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!in_bounds(x, y)) return false;
    const std::size_t idx = (static_cast<std::size_t>(y) * m_width + x) * 4;
    m_pixels[idx] = r;
    m_pixels[idx + 1] = g;
    m_pixels[idx + 2] = b;
    m_pixels[idx + 3] = a;
    return true;
}

Color FramebufferRGBA8::get_pixel(uint32_t x, uint32_t y) const {
    if (!in_bounds(x, y)) return Color::transparent();
    const std::size_t idx = (static_cast<std::size_t>(y) * m_width + x) * 4;
    return Color{
        m_pixels[idx] / 255.0f,
        m_pixels[idx + 1] / 255.0f,
        m_pixels[idx + 2] / 255.0f,
        m_pixels[idx + 3] / 255.0f
    };
}

void FramebufferRGBA8::clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    for (std::size_t i = 0; i < m_pixels.size(); i += 4) {
        m_pixels[i] = r;
        m_pixels[i + 1] = g;
        m_pixels[i + 2] = b;
        m_pixels[i + 3] = a;
    }
}

bool FramebufferRGBA8::save_png(const std::filesystem::path& path) const {
    if (m_width == 0 || m_height == 0) return false;
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }
    return stbi_write_png(path.string().c_str(),
                          static_cast<int>(m_width),
                          static_cast<int>(m_height),
                          4,
                          m_pixels.data(),
                          static_cast<int>(m_width * 4)) != 0;
}

// ---------------------------------------------------------------------------
// FramebufferRGBA16 implementation
// ---------------------------------------------------------------------------
FramebufferRGBA16::FramebufferRGBA16(uint32_t width, uint32_t height)
    : m_width(width), m_height(height) {
    m_pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);
    clear();
}

void FramebufferRGBA16::reset(uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;
    m_pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);
}

bool FramebufferRGBA16::in_bounds(uint32_t x, uint32_t y) const {
    return x < m_width && y < m_height;
}

bool FramebufferRGBA16::set_pixel(uint32_t x, uint32_t y, uint16_t r, uint16_t g, uint16_t b, uint16_t a) {
    if (!in_bounds(x, y)) return false;
    const std::size_t idx = (static_cast<std::size_t>(y) * m_width + x) * 4;
    m_pixels[idx] = r;
    m_pixels[idx + 1] = g;
    m_pixels[idx + 2] = b;
    m_pixels[idx + 3] = a;
    return true;
}

Color FramebufferRGBA16::get_pixel(uint32_t x, uint32_t y) const {
    if (!in_bounds(x, y)) return Color::transparent();
    const std::size_t idx = (static_cast<std::size_t>(y) * m_width + x) * 4;
    return Color{
        m_pixels[idx] / 65535.0f,
        m_pixels[idx + 1] / 65535.0f,
        m_pixels[idx + 2] / 65535.0f,
        m_pixels[idx + 3] / 65535.0f
    };
}

void FramebufferRGBA16::clear(uint16_t r, uint16_t g, uint16_t b, uint16_t a) {
    for (std::size_t i = 0; i < m_pixels.size(); i += 4) {
        m_pixels[i] = r;
        m_pixels[i + 1] = g;
        m_pixels[i + 2] = b;
        m_pixels[i + 3] = a;
    }
}

bool FramebufferRGBA16::save_png(const std::filesystem::path& path) const {
    if (m_width == 0 || m_height == 0) return false;
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }
    // Convert RGBA16 to RGBA8 for PNG output
    std::vector<std::uint8_t> rgba8(m_pixels.size());
    for (std::size_t i = 0; i < m_pixels.size(); ++i) {
        rgba8[i] = static_cast<std::uint8_t>(m_pixels[i] >> 8);
    }
    return stbi_write_png(path.string().c_str(),
                          static_cast<int>(m_width),
                          static_cast<int>(m_height),
                          4,
                          rgba8.data(),
                          static_cast<int>(m_width * 4)) != 0;
}

} // namespace renderer2d
} // namespace tachyon
