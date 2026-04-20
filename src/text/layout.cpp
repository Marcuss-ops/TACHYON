#include "tachyon/text/layout.h"

#include "tachyon/renderer2d/text/utf8/utf8_decoder.h"
#include "tachyon/renderer2d/text/shaping/font_shaping.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <span>
#include <string_view>

#include <hb-ft.h>
#include <hb.h>

namespace tachyon::text {
namespace {

// UTF-8 decoding moved to renderer2d::text::utf8

std::uint32_t choose_scale(const BitmapFont& font, const TextStyle& style) {
    const std::uint32_t requested = style.pixel_size == 0 ? static_cast<std::uint32_t>(font.line_height()) : style.pixel_size;
    const std::uint32_t base = static_cast<std::uint32_t>(std::max(1, font.line_height()));
    return std::max<std::uint32_t>(1, requested / base);
}

// ShapedGlyphRun moved to renderer2d::text::shaping

// Font shaping moved to renderer2d::text::shaping

bool is_breakable_space(std::uint32_t codepoint) {
    return codepoint == static_cast<std::uint32_t>(' ') || codepoint == static_cast<std::uint32_t>('\t');
}

std::int32_t aligned_x_offset(TextAlignment alignment, std::uint32_t box_width, std::int32_t line_width) {
    if (box_width == 0U) {
        return 0;
    }

    const std::int32_t available = static_cast<std::int32_t>(box_width) - line_width;
    if (available <= 0) {
        return 0;
    }

    switch (alignment) {
        case TextAlignment::Center:
            return available / 2;
        case TextAlignment::Right:
            return available;
        case TextAlignment::Left:
        default:
            return 0;
    }
}

void finalize_line(
    TextLayoutResult& result,
    std::size_t start_index,
    std::size_t glyph_count,
    std::int32_t line_width,
    std::int32_t line_y,
    TextAlignment alignment,
    std::uint32_t box_width) {

    if (glyph_count == 0U) {
        return;
    }

    const std::int32_t offset_x = aligned_x_offset(alignment, box_width, line_width);
    for (std::size_t index = start_index; index < start_index + glyph_count; ++index) {
        result.glyphs[index].x += offset_x;
    }

    result.lines.push_back(TextLine{
        start_index,
        glyph_count,
        line_width,
        line_y
    });

    const std::uint32_t used_width = box_width == 0U
        ? static_cast<std::uint32_t>(std::max(0, line_width))
        : std::min(box_width, static_cast<std::uint32_t>(std::max(0, line_width + offset_x)));

    result.width = std::max(result.width, used_width);
}

} // namespace

TextRasterSurface::TextRasterSurface(std::uint32_t width, std::uint32_t height)
    : m_width(width),
      m_height(height),
      m_pixels(width * height * 4U, 0U) {
}

renderer2d::Color TextRasterSurface::get_pixel(std::uint32_t x, std::uint32_t y) const {
    if (x >= m_width || y >= m_height) {
        return {};
    }

    const std::size_t index = (static_cast<std::size_t>(y) * m_width + x) * 4U;
    return renderer2d::Color{
        m_pixels[index + 0U],
        m_pixels[index + 1U],
        m_pixels[index + 2U],
        m_pixels[index + 3U]
    };
}

void TextRasterSurface::blend_pixel(std::uint32_t x, std::uint32_t y, renderer2d::Color color, std::uint8_t alpha) {
    if (x >= m_width || y >= m_height || alpha == 0U) {
        return;
    }

    const std::size_t index = (static_cast<std::size_t>(y) * m_width + x) * 4U;
    const std::uint32_t src_alpha = (static_cast<std::uint32_t>(color.a) * alpha) / 255U;
    const std::uint32_t dst_alpha = m_pixels[index + 3U];
    const std::uint32_t out_alpha = src_alpha + ((dst_alpha * (255U - src_alpha)) / 255U);

    auto blend_channel = [&](std::uint8_t src_channel, std::size_t channel_index) {
        const std::uint32_t dst_channel = m_pixels[index + channel_index];
        if (out_alpha == 0U) {
            m_pixels[index + channel_index] = 0U;
            return;
        }

        const std::uint32_t src_premultiplied = static_cast<std::uint32_t>(src_channel) * src_alpha;
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

TextLayoutResult layout_text(
    const BitmapFont& font,
    std::string_view utf8_text,
    const TextStyle& style,
    const TextBox& text_box,
    TextAlignment alignment,
    const TextLayoutOptions& options) {

    TextLayoutResult result;
    if (!font.is_loaded()) {
        return result;
    }

    const std::uint32_t scale = choose_scale(font, style);
    const std::int32_t scaled_line_height = font.line_height() * static_cast<std::int32_t>(scale);
    const std::int32_t wrap_width = static_cast<std::int32_t>(text_box.width);

    result.scale = scale;
    result.line_height = scaled_line_height;

    const auto codepoints = renderer2d::text::utf8::decode_utf8(utf8_text);

    if (font.has_freetype_face()) {
        std::int32_t pen_x = 0;
        std::int32_t pen_y = 0;
        std::int32_t current_line_width = 0;
        std::size_t line_start = 0;
        std::size_t last_break_codepoint_index = 0;
        std::size_t last_break_glyph_index = 0;
        std::int32_t last_break_line_width = 0;
        bool have_break = false;
        bool last_was_space = true;
        std::size_t current_word_index = 0;
        const std::int32_t tracking_advance = static_cast<std::int32_t>(std::lround(options.tracking * static_cast<float>(scale)));

        for (std::size_t index = 0; index < codepoints.size();) {
            const std::uint32_t codepoint = codepoints[index];

            if (codepoint == static_cast<std::uint32_t>('\n')) {
                finalize_line(result,
                              line_start,
                              result.glyphs.size() - line_start,
                              current_line_width,
                              pen_y,
                              alignment,
                              text_box.width);
                pen_x = 0;
                current_line_width = 0;
                pen_y += scaled_line_height;
                line_start = result.glyphs.size();
                have_break = false;
                last_was_space = true;
                ++index;
                continue;
            }

            if (is_breakable_space(codepoint)) {
                const GlyphBitmap* glyph = font.find_glyph(codepoint);
                if (glyph == nullptr) {
                    ++index;
                    last_was_space = true;
                    continue;
                }

                if (!last_was_space && !result.glyphs.empty()) {
                    have_break = true;
                    last_break_codepoint_index = index;
                    last_break_glyph_index = result.glyphs.size();
                    last_break_line_width = current_line_width;
                    ++current_word_index;
                }

                const std::int32_t glyph_advance = std::max(1, glyph->advance_x) * static_cast<std::int32_t>(scale);
                result.glyphs.push_back(PositionedGlyph{
                    codepoint,
                    font.glyph_index_for_codepoint(codepoint),
                    pen_x + glyph->x_offset * static_cast<std::int32_t>(scale),
                    pen_y + (font.ascent() - static_cast<std::int32_t>(glyph->height) - glyph->y_offset) * static_cast<std::int32_t>(scale),
                    static_cast<std::int32_t>(glyph->width) * static_cast<std::int32_t>(scale),
                    static_cast<std::int32_t>(glyph->height) * static_cast<std::int32_t>(scale),
                    glyph_advance,
                    result.glyphs.size(),
                    current_word_index,
                    true
                });

                pen_x += glyph_advance;
                if (tracking_advance != 0) {
                    pen_x += tracking_advance;
                }
                current_line_width = pen_x;
                last_was_space = true;
                ++index;
                continue;
            }

            const std::size_t run_start = index;
            while (index < codepoints.size() &&
                   codepoints[index] != static_cast<std::uint32_t>('\n') &&
                   !is_breakable_space(codepoints[index])) {
                ++index;
            }

            const std::vector<std::uint32_t> run_span(codepoints.begin() + run_start, codepoints.begin() + index);
            const renderer2d::text::shaping::ShapedGlyphRun shaped = renderer2d::text::shaping::shape_run_with_harfbuzz(font, run_span, scale);
            if (!last_was_space && !result.glyphs.empty()) {
                ++current_word_index;
            }

            if (wrap_width > 0 && text_box.multiline && options.word_wrap && pen_x > 0 && (pen_x + shaped.width) > wrap_width) {
                if (have_break && last_break_glyph_index > line_start) {
                    result.glyphs.resize(last_break_glyph_index);
                    finalize_line(result,
                                  line_start,
                                  last_break_glyph_index - line_start,
                                  last_break_line_width,
                                  pen_y,
                                  alignment,
                                  text_box.width);
                    index = last_break_codepoint_index;
                } else {
                    finalize_line(result,
                                  line_start,
                                  result.glyphs.size() - line_start,
                                  current_line_width,
                                  pen_y,
                                  alignment,
                                  text_box.width);
                }
                pen_x = 0;
                current_line_width = 0;
                pen_y += scaled_line_height;
                line_start = result.glyphs.size();
                have_break = false;
                last_was_space = true;
                continue;
            }

            for (const auto& glyph_run : shaped.glyphs) {
                const GlyphBitmap* glyph = font.find_glyph_by_index(glyph_run.font_glyph_index);
                if (glyph == nullptr) {
                    continue;
                }

                result.glyphs.push_back(PositionedGlyph{
                    glyph_run.codepoint,
                    glyph_run.font_glyph_index,
                    pen_x + glyph->x_offset * static_cast<std::int32_t>(scale) + glyph_run.offset_x,
                    pen_y + (font.ascent() - static_cast<std::int32_t>(glyph->height) - glyph->y_offset) * static_cast<std::int32_t>(scale) + glyph_run.offset_y,
                    static_cast<std::int32_t>(glyph->width) * static_cast<std::int32_t>(scale),
                    static_cast<std::int32_t>(glyph->height) * static_cast<std::int32_t>(scale),
                    glyph_run.advance_x,
                    result.glyphs.size(),
                    current_word_index,
                    false
                });

                pen_x += glyph_run.advance_x;
                if (tracking_advance != 0) {
                    pen_x += tracking_advance;
                }
                current_line_width = pen_x;
            }

            last_was_space = false;
        }

        finalize_line(result,
                      line_start,
                      result.glyphs.size() - line_start,
                      current_line_width,
                      pen_y,
                      alignment,
                      text_box.width);

        if (!result.lines.empty()) {
            result.height = static_cast<std::uint32_t>(result.lines.back().y + scaled_line_height);
        } else {
            result.height = static_cast<std::uint32_t>(scaled_line_height);
        }

        if (text_box.height > 0U) {
            result.height = std::min(result.height, text_box.height);
        }

        if (result.width == 0U) {
            result.width = text_box.width > 0U ? text_box.width : 0U;
        }

        return result;
    }

    std::int32_t pen_x = 0;
    std::int32_t pen_y = 0;
    std::int32_t current_line_width = 0;
    std::size_t line_start = 0;
    std::size_t last_break_codepoint_index = 0;
    std::size_t last_break_glyph_index = 0;
    std::int32_t last_break_line_width = 0;
    bool have_break = false;
    bool last_was_space = true;
    std::size_t current_word_index = 0;
    const std::int32_t tracking_advance = static_cast<std::int32_t>(std::lround(options.tracking * static_cast<float>(scale)));

    for (std::size_t index = 0; index < codepoints.size(); ++index) {
        const std::uint32_t codepoint = codepoints[index];

        if (codepoint == static_cast<std::uint32_t>('\n')) {
            finalize_line(result,
                          line_start,
                          result.glyphs.size() - line_start,
                          current_line_width,
                          pen_y,
                          alignment,
                          text_box.width);
            pen_x = 0;
            current_line_width = 0;
            pen_y += scaled_line_height;
            line_start = result.glyphs.size();
            have_break = false;
            last_was_space = true;
            continue;
        }

        const GlyphBitmap* glyph = font.find_glyph(codepoint);
        if (glyph == nullptr) {
            continue;
        }

        const bool whitespace = is_breakable_space(codepoint);
        if (!whitespace && last_was_space && !result.glyphs.empty()) {
            ++current_word_index;
        }
        if (!whitespace && result.glyphs.empty()) {
            current_word_index = 0;
        }

        const std::int32_t glyph_advance = std::max(1, glyph->advance_x) * static_cast<std::int32_t>(scale);
        
        // Apply kerning
        std::int32_t kerning_offset = 0;
        if (index > 0 && !last_was_space) {
            kerning_offset = font.get_kerning(codepoints[index - 1], codepoint) * static_cast<std::int32_t>(scale);
        }
        
        const bool should_wrap =
            text_box.multiline &&
            options.word_wrap &&
            wrap_width > 0 &&
            pen_x > 0 &&
            (pen_x + glyph_advance + kerning_offset) > wrap_width &&
            !whitespace;

        if (should_wrap) {
            if (have_break && last_break_glyph_index > line_start) {
                result.glyphs.resize(last_break_glyph_index);
                finalize_line(result,
                              line_start,
                              last_break_glyph_index - line_start,
                              last_break_line_width,
                              pen_y,
                              alignment,
                              text_box.width);
                index = last_break_codepoint_index;
            } else {
                finalize_line(result,
                              line_start,
                              result.glyphs.size() - line_start,
                              current_line_width,
                              pen_y,
                              alignment,
                              text_box.width);
            }
            pen_x = 0;
            current_line_width = 0;
            pen_y += scaled_line_height;
            line_start = result.glyphs.size();
            have_break = false;
            last_was_space = true;
            current_word_index = 0;
            continue;
        }

        pen_x += kerning_offset;

        const std::int32_t glyph_width = static_cast<std::int32_t>(glyph->width) * static_cast<std::int32_t>(scale);
        const std::int32_t glyph_height = static_cast<std::int32_t>(glyph->height) * static_cast<std::int32_t>(scale);
        const std::int32_t draw_x = pen_x + glyph->x_offset * static_cast<std::int32_t>(scale);

        const std::int32_t draw_y =
            pen_y +
            (font.ascent() - glyph->height - glyph->y_offset) * static_cast<std::int32_t>(scale);

        result.glyphs.push_back(PositionedGlyph{
            codepoint,
            font.glyph_index_for_codepoint(codepoint),
            draw_x,
            draw_y,
            glyph_width,
            glyph_height,
            glyph_advance,
            result.glyphs.size(),
            current_word_index,
            whitespace
        });

        pen_x += glyph_advance;
        if (tracking_advance != 0) {
            pen_x += tracking_advance;
        }
        current_line_width = pen_x;

        if (whitespace) {
            have_break = true;
            last_break_codepoint_index = index;
            last_break_glyph_index = result.glyphs.size();
            last_break_line_width = current_line_width;
        }

        last_was_space = whitespace;
    }

    finalize_line(result,
                  line_start,
                  result.glyphs.size() - line_start,
                  current_line_width,
                  pen_y,
                  alignment,
                  text_box.width);

    if (!result.lines.empty()) {
        result.height = static_cast<std::uint32_t>(result.lines.back().y + scaled_line_height);
    } else {
        result.height = static_cast<std::uint32_t>(scaled_line_height);
    }

    if (text_box.height > 0U) {
        result.height = std::min(result.height, text_box.height);
    }

    if (result.width == 0U) {
        result.width = text_box.width > 0U ? text_box.width : 0U;
    }

    return result;
}

TextRasterSurface rasterize_text_rgba(
    const BitmapFont& font,
    std::string_view utf8_text,
    const TextStyle& style,
    const TextBox& text_box,
    TextAlignment alignment,
    const TextLayoutOptions& layout_options,
    const TextAnimationOptions& animation) {

    const TextLayoutResult layout = layout_text(font, utf8_text, style, text_box, alignment, layout_options);
    TextRasterSurface surface(layout.width, layout.height);

    if (!font.is_loaded() || layout.width == 0U || layout.height == 0U) {
        return surface;
    }

    for (const PositionedGlyph& positioned : layout.glyphs) {
        const GlyphBitmap* glyph = font.has_freetype_face()
            ? font.find_glyph_by_index(positioned.font_glyph_index)
            : font.find_scaled_glyph(positioned.codepoint, layout.scale);
        if (glyph == nullptr || glyph->width == 0U || glyph->height == 0U) {
            continue;
        }

        float glyph_offset_x = 0.0f;
        float glyph_offset_y = 0.0f;
        float glyph_scale = 1.0f;
        float glyph_opacity = 1.0f;
        if (animation.enabled) {
            const float phase_seconds = animation.time_seconds - static_cast<float>(positioned.glyph_index) * 0.1f;
            const float wave_period = std::max(0.001f, animation.wave_period_seconds);
            const float wave_phase = (phase_seconds / wave_period) * 6.28318530717958647692f;

            glyph_offset_x = animation.per_glyph_offset_x * static_cast<float>(positioned.glyph_index) + std::sin(wave_phase) * animation.wave_amplitude_x;
            glyph_offset_y = animation.per_glyph_offset_y * static_cast<float>(positioned.glyph_index) + std::cos(wave_phase) * animation.wave_amplitude_y;
            glyph_scale = std::max(0.05f, 1.0f + animation.per_glyph_scale_delta * static_cast<float>(positioned.glyph_index));
            glyph_opacity = std::clamp(1.0f - animation.per_glyph_opacity_drop * static_cast<float>(positioned.glyph_index), 0.0f, 1.0f);
        }

        const std::int32_t base_x = positioned.x + static_cast<std::int32_t>(std::lround(glyph_offset_x));
        const std::int32_t base_y = positioned.y + static_cast<std::int32_t>(std::lround(glyph_offset_y));
        const std::uint32_t target_width = static_cast<std::uint32_t>(std::max(1, static_cast<std::int32_t>(std::lround(static_cast<float>(glyph->width) * glyph_scale))));
        const std::uint32_t target_height = static_cast<std::uint32_t>(std::max(1, static_cast<std::int32_t>(std::lround(static_cast<float>(glyph->height) * glyph_scale))));

        for (std::uint32_t target_y = 0; target_y < target_height; ++target_y) {
            const std::uint32_t source_y = std::min(glyph->height - 1U, static_cast<std::uint32_t>(std::floor(static_cast<float>(target_y) / glyph_scale)));
            for (std::uint32_t target_x = 0; target_x < target_width; ++target_x) {
                const std::uint32_t source_x = std::min(glyph->width - 1U, static_cast<std::uint32_t>(std::floor(static_cast<float>(target_x) / glyph_scale)));
                const std::uint8_t alpha = glyph->alpha_mask[source_y * glyph->width + source_x];
                if (alpha == 0U || glyph_opacity <= 0.0f) {
                    continue;
                }

                const std::uint8_t animated_alpha = static_cast<std::uint8_t>(std::clamp(std::lround(static_cast<float>(alpha) * glyph_opacity), 0L, 255L));
                const std::int32_t out_x = base_x + static_cast<std::int32_t>(target_x);
                const std::int32_t out_y = base_y + static_cast<std::int32_t>(target_y);

                if (out_x < 0 || out_y < 0) {
                    continue;
                }

                surface.blend_pixel(static_cast<std::uint32_t>(out_x),
                                    static_cast<std::uint32_t>(out_y),
                                    style.fill_color,
                                    animated_alpha);
            }
        }
    }

    return surface;
}

} // namespace tachyon::text
