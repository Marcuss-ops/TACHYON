#include "tachyon/text/font.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace tachyon::text {
namespace {

std::string trim_copy(const std::string& value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1U);
}

bool starts_with(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

int hex_nibble(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'A' && ch <= 'F') {
        return 10 + (ch - 'A');
    }
    if (ch >= 'a' && ch <= 'f') {
        return 10 + (ch - 'a');
    }
    return -1;
}

struct GlyphBuilder {
    bool active{false};
    std::uint32_t encoding{0};
    std::int32_t width{0};
    std::int32_t height{0};
    std::int32_t x_offset{0};
    std::int32_t y_offset{0};
    std::int32_t advance_x{0};
    std::vector<std::string> bitmap_rows;
};

} // namespace

std::optional<std::uint32_t> BitmapFont::parse_hex_row(const std::string& line) {
    const auto cleaned = trim_copy(line);
    if (cleaned.empty()) {
        return std::nullopt;
    }

    std::uint32_t value = 0;
    std::stringstream stream;
    stream << std::hex << cleaned;
    stream >> value;
    if (stream.fail()) {
        return std::nullopt;
    }
    return value;
}

bool BitmapFont::load_bdf(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return false;
    }

    m_loaded = false;
    m_ascent = 0;
    m_descent = 0;
    m_line_height = 0;
    m_default_advance = 0;
    m_glyphs.clear();

    GlyphBuilder builder;
    bool reading_bitmap = false;
    std::string line;

    while (std::getline(input, line)) {
        const std::string trimmed = trim_copy(line);
        if (trimmed.empty()) {
            continue;
        }

        if (starts_with(trimmed, "FONT_ASCENT ")) {
            m_ascent = std::stoi(trimmed.substr(12));
            continue;
        }

        if (starts_with(trimmed, "FONT_DESCENT ")) {
            m_descent = std::stoi(trimmed.substr(13));
            continue;
        }

        if (starts_with(trimmed, "FONTBOUNDINGBOX ")) {
            std::istringstream stream(trimmed.substr(16));
            int global_width = 0;
            int global_height = 0;
            int x_offset = 0;
            int y_offset = 0;
            stream >> global_width >> global_height >> x_offset >> y_offset;
            if (m_line_height == 0) {
                m_line_height = global_height;
            }
            if (m_default_advance == 0) {
                m_default_advance = std::max(1, global_width);
            }
            continue;
        }

        if (starts_with(trimmed, "STARTCHAR ")) {
            builder = {};
            builder.active = true;
            reading_bitmap = false;
            continue;
        }

        if (!builder.active) {
            continue;
        }

        if (starts_with(trimmed, "ENCODING ")) {
            builder.encoding = static_cast<std::uint32_t>(std::stoul(trimmed.substr(9)));
            continue;
        }

        if (starts_with(trimmed, "DWIDTH ")) {
            std::istringstream stream(trimmed.substr(7));
            stream >> builder.advance_x;
            continue;
        }

        if (starts_with(trimmed, "BBX ")) {
            std::istringstream stream(trimmed.substr(4));
            stream >> builder.width >> builder.height >> builder.x_offset >> builder.y_offset;
            continue;
        }

        if (trimmed == "BITMAP") {
            reading_bitmap = true;
            builder.bitmap_rows.clear();
            continue;
        }

        if (trimmed == "ENDCHAR") {
            GlyphBitmap glyph;
            glyph.width = static_cast<std::uint32_t>(std::max(0, builder.width));
            glyph.height = static_cast<std::uint32_t>(std::max(0, builder.height));
            glyph.x_offset = builder.x_offset;
            glyph.y_offset = builder.y_offset;
            glyph.advance_x = builder.advance_x > 0 ? builder.advance_x : std::max(1, builder.width);
            glyph.alpha_mask.assign(glyph.width * glyph.height, 0U);

            for (std::size_t row = 0; row < builder.bitmap_rows.size() && row < glyph.height; ++row) {
                const std::string bitmap_row = trim_copy(builder.bitmap_rows[row]);
                std::vector<std::uint8_t> row_bits;
                row_bits.reserve(bitmap_row.size() * 4U);

                for (char ch : bitmap_row) {
                    const int nibble = hex_nibble(ch);
                    if (nibble < 0) {
                        continue;
                    }
                    row_bits.push_back((nibble & 0x8) != 0 ? 1U : 0U);
                    row_bits.push_back((nibble & 0x4) != 0 ? 1U : 0U);
                    row_bits.push_back((nibble & 0x2) != 0 ? 1U : 0U);
                    row_bits.push_back((nibble & 0x1) != 0 ? 1U : 0U);
                }

                for (std::uint32_t column = 0; column < glyph.width && column < row_bits.size(); ++column) {
                    glyph.alpha_mask[row * glyph.width + column] = row_bits[column] != 0U ? 255U : 0U;
                }
            }

            m_default_advance = std::max(m_default_advance, glyph.advance_x);
            m_glyphs[builder.encoding] = std::move(glyph);
            builder = {};
            reading_bitmap = false;
            continue;
        }

        if (reading_bitmap) {
            builder.bitmap_rows.push_back(trimmed);
        }
    }

    if (m_line_height == 0) {
        m_line_height = std::max(1, m_ascent + m_descent);
    }
    if (m_line_height == 0) {
        m_line_height = 1;
    }
    if (m_default_advance == 0) {
        m_default_advance = 1;
    }

    m_loaded = !m_glyphs.empty();
    return m_loaded;
}

const GlyphBitmap* BitmapFont::find_glyph(std::uint32_t codepoint) const {
    const auto it = m_glyphs.find(codepoint);
    if (it != m_glyphs.end()) {
        return &it->second;
    }
    return fallback_glyph();
}

const GlyphBitmap* BitmapFont::fallback_glyph() const {
    const auto question_mark = m_glyphs.find(static_cast<std::uint32_t>('?'));
    if (question_mark != m_glyphs.end()) {
        return &question_mark->second;
    }

    const auto uppercase_n = m_glyphs.find(static_cast<std::uint32_t>('N'));
    if (uppercase_n != m_glyphs.end()) {
        return &uppercase_n->second;
    }

    if (!m_glyphs.empty()) {
        return &m_glyphs.begin()->second;
    }

    return nullptr;
}

} // namespace tachyon::text
