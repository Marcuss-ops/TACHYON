#pragma once

#include "tachyon/text/font.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace tachyon::text {

enum class TextAlignment {
    Left,
    Center,
    Right
};

struct TextStyle {
    std::uint32_t pixel_size{0};
    renderer2d::Color fill_color{renderer2d::Color::white()};
};

struct TextLayoutOptions {
    float tracking{0.0f};
    bool word_wrap{true};
};

struct TextAnimationOptions {
    bool enabled{false};
    float time_seconds{0.0f};
    float per_glyph_offset_x{0.0f};
    float per_glyph_offset_y{0.0f};
    float per_glyph_scale_delta{0.0f};
    float per_glyph_opacity_drop{0.0f};
    float wave_amplitude_x{0.0f};
    float wave_amplitude_y{0.0f};
    float wave_period_seconds{1.0f};
};

struct TextBox {
    std::uint32_t width{0};
    std::uint32_t height{0};
    bool multiline{true};
};

struct PositionedGlyph {
    std::uint32_t codepoint{0};
    std::int32_t x{0};
    std::int32_t y{0};
    std::int32_t width{0};
    std::int32_t height{0};
    std::int32_t advance_x{0};
    std::size_t glyph_index{0};
    std::size_t word_index{0};
    bool whitespace{false};
};

struct TextLine {
    std::size_t glyph_start_index{0};
    std::size_t glyph_count{0};
    std::int32_t width{0};
    std::int32_t y{0};
};

struct TextLayoutResult {
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::uint32_t scale{1};
    std::int32_t line_height{0};
    std::vector<TextLine> lines;
    std::vector<PositionedGlyph> glyphs;
};

class TextRasterSurface {
public:
    TextRasterSurface() = default;
    TextRasterSurface(std::uint32_t width, std::uint32_t height);

    std::uint32_t width() const { return m_width; }
    std::uint32_t height() const { return m_height; }
    const std::vector<std::uint8_t>& rgba_pixels() const { return m_pixels; }

    renderer2d::Color get_pixel(std::uint32_t x, std::uint32_t y) const;
    bool save_png(const std::filesystem::path& path) const;

private:
    friend TextRasterSurface rasterize_text_rgba(const BitmapFont&, std::string_view, const TextStyle&, const TextBox&, TextAlignment, const TextLayoutOptions&, const TextAnimationOptions&);

    void blend_pixel(std::uint32_t x, std::uint32_t y, renderer2d::Color color, std::uint8_t alpha);

    std::uint32_t m_width{0};
    std::uint32_t m_height{0};
    std::vector<std::uint8_t> m_pixels;
};

TextLayoutResult layout_text(
    const BitmapFont& font,
    std::string_view utf8_text,
    const TextStyle& style,
    const TextBox& text_box,
    TextAlignment alignment,
    const TextLayoutOptions& options = {});

TextRasterSurface rasterize_text_rgba(
    const BitmapFont& font,
    std::string_view utf8_text,
    const TextStyle& style,
    const TextBox& text_box,
    TextAlignment alignment,
    const TextLayoutOptions& layout_options = {},
    const TextAnimationOptions& animation = {});

} // namespace tachyon::text
