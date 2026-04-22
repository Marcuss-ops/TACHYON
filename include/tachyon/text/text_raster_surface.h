#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/text/glyph/glyph_bitmap.h"
#include <vector>
#include <cstdint>
#include <filesystem>

namespace tachyon::text {

/**
 * A dedicated surface for rasterizing text glyphs with alpha blending.
 */
class TextRasterSurface {
public:
    TextRasterSurface(std::uint32_t width, std::uint32_t height);

    std::uint32_t width() const { return m_width; }
    std::uint32_t height() const { return m_height; }

    tachyon::renderer2d::Color get_pixel(std::uint32_t x, std::uint32_t y) const;
    void blend_pixel(std::uint32_t x, std::uint32_t y, tachyon::renderer2d::Color color, std::uint8_t alpha);
    void render_glyph(const tachyon::text::GlyphBitmap& glyph, int tx, int ty, int tw, int th, tachyon::renderer2d::Color gc);

    bool save_png(const std::filesystem::path& path) const;

    const std::vector<std::uint8_t>& rgba_pixels() const { return m_pixels; }

private:
    std::uint32_t m_width;
    std::uint32_t m_height;
    std::vector<std::uint8_t> m_pixels;
};

} // namespace tachyon::text
