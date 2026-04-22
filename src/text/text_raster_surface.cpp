#include "tachyon/text/text_raster_surface.h"
#include <stb_image_write.h>
#include <algorithm>

namespace tachyon::text {

void TextRasterSurface::render_glyph(const tachyon::text::GlyphBitmap& glyph, int tx, int ty, int tw, int th, tachyon::renderer2d::Color gc) {
    (void)glyph; (void)tx; (void)ty; (void)tw; (void)th; (void)gc;
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
