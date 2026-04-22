#include "tachyon/text/rendering/text_raster_surface.h"
#include <stb_image_write.h>
#include <algorithm>

namespace tachyon::text {

void TextRasterSurface::render_glyph(const tachyon::text::GlyphBitmap& glyph, int tx, int ty, int tw, int th, tachyon::renderer2d::Color gc) {
    if (tw <= 0 || th <= 0 || glyph.width == 0U || glyph.height == 0U || glyph.alpha_mask.empty()) return;
    for (int y = 0; y < th; ++y) {
        for (int x = 0; x < tw; ++x) {
            const float sx = static_cast<float>(x) * static_cast<float>(glyph.width) / static_cast<float>(tw);
            const float sy = static_cast<float>(y) * static_cast<float>(glyph.height) / static_cast<float>(th);
            const std::uint32_t gx = std::min<std::uint32_t>(static_cast<std::uint32_t>(sx), glyph.width - 1U);
            const std::uint32_t gy = std::min<std::uint32_t>(static_cast<std::uint32_t>(sy), glyph.height - 1U);
            const std::size_t glyph_index = static_cast<std::size_t>(gy) * glyph.width + gx;
            const std::uint8_t alpha = glyph_index < glyph.alpha_mask.size() ? glyph.alpha_mask[glyph_index] : 0U;
            blend_pixel(static_cast<std::uint32_t>(tx + x), static_cast<std::uint32_t>(ty + y), gc, alpha);
        }
    }
}

void TextRasterSurface::draw_rect(int x, int y, int w, int h, tachyon::renderer2d::Color color) {
    if (w <= 0 || h <= 0) return;
    for (int i = 0; i < w; ++i) {
        blend_pixel(static_cast<std::uint32_t>(x + i), static_cast<std::uint32_t>(y), color, 255);
        blend_pixel(static_cast<std::uint32_t>(x + i), static_cast<std::uint32_t>(y + h - 1), color, 255);
    }
    for (int i = 0; i < h; ++i) {
        blend_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y + i), color, 255);
        blend_pixel(static_cast<std::uint32_t>(x + w - 1), static_cast<std::uint32_t>(y + i), color, 255);
    }
}

void TextRasterSurface::draw_line(int x0, int y0, int x1, int y1, tachyon::renderer2d::Color color) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (true) {
        blend_pixel(static_cast<std::uint32_t>(x0), static_cast<std::uint32_t>(y0), color, 255);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

TextRasterSurface::TextRasterSurface(std::uint32_t width, std::uint32_t height)
    : m_width(width),
      m_height(height),
      m_pixels(width * height * 4U, 0U) {
}

tachyon::renderer2d::Color TextRasterSurface::get_pixel(std::uint32_t x, std::uint32_t y) const {
    if (x >= m_width || y >= m_height) {
        return {};
    }

    const std::size_t index = (static_cast<std::size_t>(y) * m_width + x) * 4U;
    return tachyon::renderer2d::Color{
        static_cast<float>(m_pixels[index + 0U]) / 255.0f,
        static_cast<float>(m_pixels[index + 1U]) / 255.0f,
        static_cast<float>(m_pixels[index + 2U]) / 255.0f,
        static_cast<float>(m_pixels[index + 3U]) / 255.0f
    };
}

void TextRasterSurface::blend_pixel(std::uint32_t x, std::uint32_t y, tachyon::renderer2d::Color color, std::uint8_t alpha) {
    if (x >= m_width || y >= m_height || alpha == 0U) {
        return;
    }

    const std::size_t index = (static_cast<std::size_t>(y) * m_width + x) * 4U;
    const std::uint32_t src_alpha = static_cast<std::uint32_t>(std::clamp(color.a * 255.0f, 0.0f, 255.0f)) * alpha / 255U;
    const std::uint32_t dst_alpha = m_pixels[index + 3U];
    const std::uint32_t out_alpha = src_alpha + ((dst_alpha * (255U - src_alpha)) / 255U);

    auto blend_channel = [&](float src_channel_f, std::size_t channel_index) {
        const std::uint32_t src_channel = static_cast<std::uint32_t>(std::clamp(src_channel_f * 255.0f, 0.0f, 255.0f));
        const std::uint32_t dst_channel = m_pixels[index + channel_index];
        if (out_alpha == 0U) {
            m_pixels[index + channel_index] = 0U;
            return;
        }

        const std::uint32_t src_premultiplied = src_channel * src_alpha;
        const std::uint32_t dst_premultiplied = dst_channel * dst_alpha * (255U - src_alpha) / 255U;
        m_pixels[index + channel_index] = static_cast<std::uint8_t>((src_premultiplied + dst_premultiplied) / out_alpha);
    };

    blend_channel(color.r, 0U);
    blend_channel(color.g, 1U);
    blend_channel(color.b, 2U);
    m_pixels[index + 3U] = static_cast<std::uint8_t>(out_alpha);
}

bool TextRasterSurface::save_png(const std::filesystem::path& path) const {
    if (m_width == 0U || m_height == 0U) {
        return false;
    }

    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }

    return stbi_write_png(path.string().c_str(),
                          static_cast<int>(m_width),
                          static_cast<int>(m_height),
                          4,
                          m_pixels.data(),
                          static_cast<int>(m_width * 4U)) != 0;
}

} // namespace tachyon::text
