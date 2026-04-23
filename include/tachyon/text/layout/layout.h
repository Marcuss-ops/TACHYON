#pragma once

#include "tachyon/text/fonts/font.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/text/rendering/text_raster_surface.h"
#include "tachyon/text/core/layout/resolved_text_layout.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace tachyon::text {

class FontRegistry;

enum class TextAlignment {
    Left,
    Center,
    Right,
    Justify
};

struct TextFeature {
    std::string tag;
    std::uint32_t value{1};
};

struct TextStyle {
    std::uint32_t pixel_size{0};
    renderer2d::Color fill_color{renderer2d::Color::white()};
    std::vector<TextFeature> features;
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

struct TextHighlightSpan {
    std::size_t start_glyph{0};
    std::size_t end_glyph{0}; // exclusive
    renderer2d::Color color{renderer2d::Color{255, 236, 59, 96}};
    std::int32_t padding_x{3};
    std::int32_t padding_y{2};
};

struct TextBox {
    std::uint32_t width{0};
    std::uint32_t height{0};
    bool multiline{true};
};

struct PositionedGlyph {
    std::uint32_t codepoint{0};
    std::uint32_t font_glyph_index{0};
    std::uint64_t font_id{0};
    std::int32_t x{0};
    std::int32_t y{0};
    std::int32_t width{0};
    std::int32_t height{0};
    std::int32_t advance_x{0};
    std::size_t glyph_index{0};
    std::size_t word_index{0};
    std::size_t cluster_index{0};
    std::size_t cluster_codepoint_start{0};
    std::size_t cluster_codepoint_count{1};
    bool whitespace{false};
    bool is_rtl{false};
    const Font* resolved_font{nullptr};

    // Animatable properties added to support TextAnimatorPipeline
    math::Vector2 scale{1.0f, 1.0f};
    float rotation{0.0f};
    float opacity{1.0f};
    ColorSpec fill_color{255, 255, 255, 255};
};

struct TextLine {
    std::size_t glyph_start_index{0};
    std::size_t glyph_count{0};
    std::int32_t width{0};
    std::int32_t y{0};
};

struct TextHitTestResult {
    std::size_t codepoint_index{0};
    std::size_t cluster_index{0};
    bool is_leading_edge{true};
    bool inside_text{false};
};

struct TextLayoutResult : public ResolvedTextLayout {
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::uint32_t scale{1};
    std::int32_t line_height{0};
    std::vector<TextLine> lines;
    std::vector<PositionedGlyph> glyphs;

    TextHitTestResult hit_test(std::int32_t x, std::int32_t y) const;
    renderer2d::RectI get_caret_rect(std::size_t codepoint_index) const;
};

TextLayoutResult layout_text(
    const BitmapFont& font,
    std::string_view utf8_text,
    const TextStyle& style,
    const TextBox& text_box,
    TextAlignment alignment,
    const TextLayoutOptions& options = {});

TextLayoutResult layout_text(
    const FontRegistry& registry,
    const std::string& font_name,
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

/// Overload with per-character Text Animator support.
/// Each animator in @p animators is evaluated at @p time_seconds.
/// The selector coverage is blended per glyph before rasterization.
TextRasterSurface rasterize_text_rgba(
    const BitmapFont& font,
    std::string_view utf8_text,
    const TextStyle& style,
    const TextBox& text_box,
    TextAlignment alignment,
    float time_seconds,
    std::span<const TextAnimatorSpec> animators,
    const TextLayoutOptions& layout_options = {});

TextRasterSurface rasterize_text_rgba(
    const BitmapFont& font,
    std::string_view utf8_text,
    const TextStyle& style,
    const TextBox& text_box,
    TextAlignment alignment,
    std::span<const TextHighlightSpan> highlights,
    const TextLayoutOptions& layout_options = {},
    const TextAnimationOptions& animation = {});

struct TextOutlineOptions {
    float width{0.0f};
    renderer2d::Color color{renderer2d::Color::black()};
};

/// Overload with outline support for subtitle burn-in
TextRasterSurface rasterize_text_rgba(
    const BitmapFont& font,
    std::string_view utf8_text,
    const TextStyle& style,
    const TextBox& text_box,
    TextAlignment alignment,
    const TextOutlineOptions& outline);

TextRasterSurface rasterize_layout_debug(
    const TextLayoutResult& layout);

} // namespace tachyon::text
