#include "tachyon/text/layout.h"

#include "stb_image_write.h"

#include <algorithm>
#include <filesystem>
#include <string_view>

namespace tachyon::text {
namespace {

std::vector<std::uint32_t> decode_utf8(std::string_view text) {
    std::vector<std::uint32_t> codepoints;
    codepoints.reserve(text.size());

    for (std::size_t i = 0; i < text.size();) {
        const unsigned char lead = static_cast<unsigned char>(text[i]);
        if ((lead & 0x80U) == 0U) {
            codepoints.push_back(lead);
            ++i;
            continue;
        }

        if ((lead & 0xE0U) == 0xC0U && i + 1 < text.size()) {
            const auto cp =
                ((static_cast<std::uint32_t>(lead & 0x1FU)) << 6U) |
                (static_cast<std::uint32_t>(static_cast<unsigned char>(text[i + 1]) & 0x3FU));
            codepoints.push_back(cp);
            i += 2;
            continue;
        }

        if ((lead & 0xF0U) == 0xE0U && i + 2 < text.size()) {
            const auto cp =
                ((static_cast<std::uint32_t>(lead & 0x0FU)) << 12U) |
                ((static_cast<std::uint32_t>(static_cast<unsigned char>(text[i + 1]) & 0x3FU)) << 6U) |
                (static_cast<std::uint32_t>(static_cast<unsigned char>(text[i + 2]) & 0x3FU));
            codepoints.push_back(cp);
            i += 3;
            continue;
        }

        if ((lead & 0xF8U) == 0xF0U && i + 3 < text.size()) {
            const auto cp =
                ((static_cast<std::uint32_t>(lead & 0x07U)) << 18U) |
                ((static_cast<std::uint32_t>(static_cast<unsigned char>(text[i + 1]) & 0x3FU)) << 12U) |
                ((static_cast<std::uint32_t>(static_cast<unsigned char>(text[i + 2]) & 0x3FU)) << 6U) |
                (static_cast<std::uint32_t>(static_cast<unsigned char>(text[i + 3]) & 0x3FU));
            codepoints.push_back(cp);
            i += 4;
            continue;
        }

        codepoints.push_back(static_cast<std::uint32_t>('?'));
        ++i;
    }

    return codepoints;
}

std::uint32_t choose_scale(const BitmapFont& font, const TextStyle& style) {
    const std::uint32_t requested = style.pixel_size == 0 ? static_cast<std::uint32_t>(font.line_height()) : style.pixel_size;
    const std::uint32_t base = static_cast<std::uint32_t>(std::max(1, font.line_height()));
    return std::max<std::uint32_t>(1, requested / base);
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
    TextAlignment alignment) {

    TextLayoutResult result;
    if (!font.is_loaded()) {
        return result;
    }

    const std::uint32_t scale = choose_scale(font, style);
    const std::int32_t scaled_line_height = font.line_height() * static_cast<std::int32_t>(scale);
    const std::int32_t wrap_width = static_cast<std::int32_t>(text_box.width);

    result.scale = scale;
    result.line_height = scaled_line_height;

    const auto codepoints = decode_utf8(utf8_text);

    std::int32_t pen_x = 0;
    std::int32_t pen_y = 0;
    std::int32_t current_line_width = 0;
    std::size_t line_start = 0;

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
            continue;
        }

        const GlyphBitmap* glyph = font.find_glyph(codepoint);
        if (glyph == nullptr) {
            continue;
        }

        const std::int32_t glyph_advance = std::max(1, glyph->advance_x) * static_cast<std::int32_t>(scale);
        const bool should_wrap =
            text_box.multiline &&
            wrap_width > 0 &&
            pen_x > 0 &&
            (pen_x + glyph_advance) > wrap_width &&
            codepoint != static_cast<std::uint32_t>(' ');

        if (should_wrap) {
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
        }

        const std::int32_t glyph_width = static_cast<std::int32_t>(glyph->width) * static_cast<std::int32_t>(scale);
        const std::int32_t glyph_height = static_cast<std::int32_t>(glyph->height) * static_cast<std::int32_t>(scale);
        const std::int32_t draw_x = pen_x + glyph->x_offset * static_cast<std::int32_t>(scale);
        const std::int32_t draw_y =
            pen_y +
            (font.ascent() - glyph->height - glyph->y_offset) * static_cast<std::int32_t>(scale);

        result.glyphs.push_back(PositionedGlyph{
            codepoint,
            draw_x,
            draw_y,
            glyph_width,
            glyph_height,
            glyph_advance
        });

        pen_x += glyph_advance;
        current_line_width = pen_x;
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
    TextAlignment alignment) {

    const TextLayoutResult layout = layout_text(font, utf8_text, style, text_box, alignment);
    TextRasterSurface surface(layout.width, layout.height);

    if (!font.is_loaded() || layout.width == 0U || layout.height == 0U) {
        return surface;
    }

    for (const PositionedGlyph& positioned : layout.glyphs) {
        const GlyphBitmap* glyph = font.find_glyph(positioned.codepoint);
        if (glyph == nullptr || glyph->width == 0U || glyph->height == 0U) {
            continue;
        }

        for (std::uint32_t source_y = 0; source_y < glyph->height; ++source_y) {
            for (std::uint32_t source_x = 0; source_x < glyph->width; ++source_x) {
                const std::uint8_t alpha = glyph->alpha_mask[source_y * glyph->width + source_x];
                if (alpha == 0U) {
                    continue;
                }

                for (std::uint32_t dy = 0; dy < layout.scale; ++dy) {
                    for (std::uint32_t dx = 0; dx < layout.scale; ++dx) {
                        const std::int32_t out_x =
                            positioned.x + static_cast<std::int32_t>(source_x * layout.scale + dx);
                        const std::int32_t out_y =
                            positioned.y + static_cast<std::int32_t>(source_y * layout.scale + dy);

                        if (out_x < 0 || out_y < 0) {
                            continue;
                        }

                        surface.blend_pixel(static_cast<std::uint32_t>(out_x),
                                            static_cast<std::uint32_t>(out_y),
                                            style.fill_color,
                                            alpha);
                    }
                }
            }
        }
    }

    return surface;
}

} // namespace tachyon::text
