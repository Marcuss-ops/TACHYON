#include "tachyon/text/rendering/text_raster_surface.h"
#include <stb_image_write.h>
#include <algorithm>
#include <cmath>

namespace tachyon::text {

namespace {

float sample_glyph_alpha(const tachyon::text::GlyphBitmap& glyph, float src_x, float src_y) {
    if (glyph.width == 0U || glyph.height == 0U || glyph.alpha_mask.empty()) {
        return 0.0f;
    }

    const float max_x = static_cast<float>(glyph.width - 1U);
    const float max_y = static_cast<float>(glyph.height - 1U);
    src_x = std::clamp(src_x, 0.0f, max_x);
    src_y = std::clamp(src_y, 0.0f, max_y);

    const std::uint32_t x0 = static_cast<std::uint32_t>(std::floor(src_x));
    const std::uint32_t y0 = static_cast<std::uint32_t>(std::floor(src_y));
    const std::uint32_t x1 = std::min(x0 + 1U, glyph.width - 1U);
    const std::uint32_t y1 = std::min(y0 + 1U, glyph.height - 1U);

    const float fx = src_x - static_cast<float>(x0);
    const float fy = src_y - static_cast<float>(y0);

    auto alpha_at = [&](std::uint32_t x, std::uint32_t y) -> float {
        const std::size_t index = static_cast<std::size_t>(y) * glyph.width + x;
        if (index >= glyph.alpha_mask.size()) {
            return 0.0f;
        }
        return static_cast<float>(glyph.alpha_mask[index]) / 255.0f;
    };

    const float a00 = alpha_at(x0, y0);
    const float a10 = alpha_at(x1, y0);
    const float a01 = alpha_at(x0, y1);
    const float a11 = alpha_at(x1, y1);

    const float ax0 = a00 + (a10 - a00) * fx;
    const float ax1 = a01 + (a11 - a01) * fx;
    return ax0 + (ax1 - ax0) * fy;
}

} // namespace

void TextRasterSurface::render_glyph(const tachyon::text::GlyphBitmap& glyph, int tx, int ty, int tw, int th, tachyon::renderer2d::Color gc) {
    if (tw <= 0 || th <= 0 || glyph.width == 0U || glyph.height == 0U || glyph.alpha_mask.empty()) return;
    for (int y = 0; y < th; ++y) {
        const float src_y = ((static_cast<float>(y) + 0.5f) * static_cast<float>(glyph.height) / static_cast<float>(th)) - 0.5f;
        for (int x = 0; x < tw; ++x) {
            const float src_x = ((static_cast<float>(x) + 0.5f) * static_cast<float>(glyph.width) / static_cast<float>(tw)) - 0.5f;
            const float alpha = sample_glyph_alpha(glyph, src_x, src_y);
            blend_pixel(static_cast<std::uint32_t>(tx + x), static_cast<std::uint32_t>(ty + y), gc, static_cast<std::uint8_t>(std::lround(std::clamp(alpha, 0.0f, 1.0f) * 255.0f)));
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
