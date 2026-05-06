#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/text/core/low_level/glyph/glyph_bitmap.h"
namespace tachyon::text {
struct GradientSpec;
struct GradientStop;
struct ResolvedTextLayout;
struct TextGlowOptions;
struct TextShadowOptions;
struct TextStyle;
} // namespace tachyon::text

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
    ~TextRasterSurface();

    std::uint32_t width() const { return m_width; }
    std::uint32_t height() const { return m_height; }

    tachyon::renderer2d::Color get_pixel(std::uint32_t x, std::uint32_t y) const;
    void blend_pixel(std::uint32_t x, std::uint32_t y, tachyon::renderer2d::Color color, std::uint8_t alpha);
    void render_glyph(const tachyon::text::GlyphBitmap& glyph, int tx, int ty, int tw, int th, tachyon::renderer2d::Color gc);
    void draw_rect(int x, int y, int w, int h, tachyon::renderer2d::Color color);
    void draw_line(int x0, int y0, int x1, int y1, tachyon::renderer2d::Color color);

    void apply_gaussian_blur(float radius);
    void apply_shadow(const TextShadowOptions& options);
    void apply_glow(const TextGlowOptions& options);

    bool save_png(const std::filesystem::path& path) const;

    const std::vector<std::uint8_t>& rgba_pixels() const;

private:
    void ensure_pixels_current();
    void ensure_canvas_current();

    std::uint32_t m_width;
    std::uint32_t m_height;
    std::vector<std::uint8_t> m_pixels;
};

} // namespace tachyon::text
