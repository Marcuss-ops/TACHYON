#include "tachyon/renderer2d/text/glyph/glyph_loader.h"
#include "tachyon/renderer2d/text/utils/font_utils.h"

#include <fstream>
#include <sstream>
#include <algorithm>

namespace tachyon::renderer2d::text {

namespace {

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

LoadedFont GlyphLoader::load_bdf(const std::filesystem::path& path) {
    std::ifstream input(path);
    LoadedFont result;
    if (!input.is_open()) {
        result.success = false;
        return result;
    }

    GlyphBuilder builder;
    bool reading_bitmap = false;
    std::string line;

    while (std::getline(input, line)) {
        const std::string trimmed = trim_copy(line);
        if (trimmed.empty()) {
            continue;
        }

        if (starts_with(trimmed, "FONT_ASCENT ")) {
            result.metrics.ascent = std::stoi(trimmed.substr(12));
            continue;
        }

        if (starts_with(trimmed, "FONT_DESCENT ")) {
            result.metrics.descent = std::stoi(trimmed.substr(13));
            continue;
        }

        if (starts_with(trimmed, "FONTBOUNDINGBOX ")) {
            std::istringstream stream(trimmed.substr(16));
            int global_width = 0;
            int global_height = 0;
            int x_offset = 0;
            int y_offset = 0;
            stream >> global_width >> global_height >> x_offset >> y_offset;
            if (result.metrics.line_height == 0) {
                result.metrics.line_height = global_height;
            }
            if (result.metrics.default_advance == 0) {
                result.metrics.default_advance = std::max(1, global_width);
            }
            continue;
        }

        if (starts_with(trimmed, "STARTKERNING ")) {
            reading_bitmap = false;
            while (std::getline(input, line)) {
                const std::string k_trimmed = trim_copy(line);
                if (starts_with(k_trimmed, "ENDKERNING")) break;
                if (starts_with(k_trimmed, "KPT ")) {
                    std::istringstream k_stream(k_trimmed.substr(4));
                    std::uint32_t left = 0, right = 0;
                    std::int32_t amount = 0;
                    k_stream >> left >> right >> amount;
                    if (!k_stream.fail()) {
                        const std::uint64_t key = (static_cast<std::uint64_t>(left) << 32U) | static_cast<std::uint64_t>(right);
                        result.kerning_table[key] = amount;
                    }
                }
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
            glyph.pixels.assign(glyph.width * glyph.height, 0U);

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
                    glyph.pixels[row * glyph.width + column] = row_bits[column] != 0U ? 255U : 0U;
                }
            }

            result.metrics.default_advance = std::max(result.metrics.default_advance, glyph.advance_x);
            result.glyphs[builder.encoding] = std::move(glyph);
            builder = {};
            reading_bitmap = false;
            continue;
        }

        if (reading_bitmap) {
            builder.bitmap_rows.push_back(trimmed);
        }
    }

    if (result.metrics.line_height == 0) {
        result.metrics.line_height = std::max(1, result.metrics.ascent + result.metrics.descent);
    }
    if (result.metrics.line_height == 0) {
        result.metrics.line_height = 1;
    }
    if (result.metrics.default_advance == 0) {
        result.metrics.default_advance = 1;
    }

    result.success = !result.glyphs.empty();
    return result;
}

} // namespace tachyon::renderer2d::text
