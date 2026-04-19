#include "tachyon/text/font.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace tachyon::text {
namespace {

struct FTManager {
    FT_Library library{nullptr};
    FTManager() { FT_Init_FreeType(&library); }
    ~FTManager() { if (library) FT_Done_FreeType(library); }
};

FT_Library get_ft() {
    static FTManager manager;
    return manager.library;
}

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

Font::Font() {}

Font::~Font() {
    if (m_ft_face) {
        FT_Done_Face(static_cast<FT_Face>(m_ft_face));
    }
}

Font::Font(Font&& other) noexcept
    : m_loaded(other.m_loaded),
      m_is_freetype(other.m_is_freetype),
      m_ascent(other.m_ascent),
      m_descent(other.m_descent),
      m_line_height(other.m_line_height),
      m_default_advance(other.m_default_advance),
      m_glyphs(std::move(other.m_glyphs)),
      m_kerning_table(std::move(other.m_kerning_table)),
      m_ft_face(other.m_ft_face),
      m_font_data(std::move(other.m_font_data)),
      m_ft_glyph_cache(std::move(other.m_ft_glyph_cache)),
      m_ft_index_cache(std::move(other.m_ft_index_cache)),
      m_scaled_glyph_cache(std::move(other.m_scaled_glyph_cache)) {
    other.m_loaded = false;
    other.m_is_freetype = false;
    other.m_ascent = 0;
    other.m_descent = 0;
    other.m_line_height = 0;
    other.m_default_advance = 0;
    other.m_ft_face = nullptr;
}

Font& Font::operator=(Font&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (m_ft_face) {
        FT_Done_Face(static_cast<FT_Face>(m_ft_face));
        m_ft_face = nullptr;
    }

    m_loaded = other.m_loaded;
    m_is_freetype = other.m_is_freetype;
    m_ascent = other.m_ascent;
    m_descent = other.m_descent;
    m_line_height = other.m_line_height;
    m_default_advance = other.m_default_advance;
    m_glyphs = std::move(other.m_glyphs);
    m_kerning_table = std::move(other.m_kerning_table);
    m_ft_face = other.m_ft_face;
    m_font_data = std::move(other.m_font_data);
    m_ft_glyph_cache = std::move(other.m_ft_glyph_cache);
    m_ft_index_cache = std::move(other.m_ft_index_cache);
    m_scaled_glyph_cache = std::move(other.m_scaled_glyph_cache);

    other.m_loaded = false;
    other.m_is_freetype = false;
    other.m_ascent = 0;
    other.m_descent = 0;
    other.m_line_height = 0;
    other.m_default_advance = 0;
    other.m_ft_face = nullptr;
    return *this;
}

std::optional<std::uint32_t> Font::parse_hex_row(const std::string& line) {
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

std::uint64_t Font::scaled_cache_key(std::uint32_t codepoint, std::uint32_t scale) {
    return (static_cast<std::uint64_t>(codepoint) << 32U) | static_cast<std::uint64_t>(scale);
}

const GlyphBitmap* Font::scale_glyph(std::uint32_t codepoint, const GlyphBitmap& glyph, std::uint32_t scale) const {
    if (scale <= 1U) {
        return &glyph;
    }

    const std::uint64_t key = scaled_cache_key(codepoint, scale);
    const auto cache_it = m_scaled_glyph_cache.find(key);
    if (cache_it != m_scaled_glyph_cache.end()) {
        return cache_it->second.get();
    }

    auto scaled = std::make_unique<GlyphBitmap>();
    scaled->width = glyph.width * scale;
    scaled->height = glyph.height * scale;
    scaled->x_offset = glyph.x_offset * static_cast<std::int32_t>(scale);
    scaled->y_offset = glyph.y_offset * static_cast<std::int32_t>(scale);
    scaled->advance_x = glyph.advance_x * static_cast<std::int32_t>(scale);
    scaled->alpha_mask.assign(static_cast<std::size_t>(scaled->width) * static_cast<std::size_t>(scaled->height), 0U);

    if (glyph.width != 0U && glyph.height != 0U) {
        for (std::uint32_t source_y = 0; source_y < glyph.height; ++source_y) {
            for (std::uint32_t source_x = 0; source_x < glyph.width; ++source_x) {
                const std::uint8_t alpha = glyph.alpha_mask[source_y * glyph.width + source_x];
                if (alpha == 0U) {
                    continue;
                }

                for (std::uint32_t dy = 0; dy < scale; ++dy) {
                    for (std::uint32_t dx = 0; dx < scale; ++dx) {
                        const std::size_t dst_index =
                            (static_cast<std::size_t>(source_y) * scale + dy) * scaled->width +
                            (static_cast<std::size_t>(source_x) * scale + dx);
                        scaled->alpha_mask[dst_index] = alpha;
                    }
                }
            }
        }
    }

    const GlyphBitmap* scaled_ptr = scaled.get();
    m_scaled_glyph_cache.emplace(key, std::move(scaled));
    return scaled_ptr;
}

bool Font::load_bdf(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return false;
    }

    m_loaded = false;
    m_is_freetype = false;
    m_ascent = 0;
    m_descent = 0;
    m_line_height = 0;
    m_default_advance = 0;
    m_glyphs.clear();
    m_scaled_glyph_cache.clear();
    m_ft_glyph_cache.clear();
    m_ft_index_cache.clear();
    m_font_data.clear();

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
                        m_kerning_table[key] = amount;
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

bool Font::load_ttf(const std::filesystem::path& path, std::uint32_t pixel_size) {
    if (m_ft_face) {
        FT_Done_Face(static_cast<FT_Face>(m_ft_face));
        m_ft_face = nullptr;
    }

    m_loaded = false;
    m_is_freetype = false;
    m_ascent = 0;
    m_descent = 0;
    m_line_height = 0;
    m_default_advance = 0;
    m_glyphs.clear();
    m_kerning_table.clear();
    m_scaled_glyph_cache.clear();
    m_ft_glyph_cache.clear();
    m_ft_index_cache.clear();
    m_font_data.clear();

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    m_font_data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    if (m_font_data.empty()) {
        return false;
    }

    FT_Face face;
    if (FT_New_Memory_Face(get_ft(),
                           m_font_data.data(),
                           static_cast<FT_Long>(m_font_data.size()),
                           0,
                           &face)) {
        m_font_data.clear();
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, pixel_size);

    m_is_freetype = true;
    m_ft_face = face;
    m_ascent = static_cast<std::int32_t>(face->size->metrics.ascender >> 6);
    m_descent = static_cast<std::int32_t>(face->size->metrics.descender >> 6);
    m_line_height = static_cast<std::int32_t>(face->size->metrics.height >> 6);
    m_default_advance = static_cast<std::int32_t>(face->max_advance_width >> 6);
    
    m_ft_glyph_cache.clear();
    m_ft_index_cache.clear();
    m_loaded = true;
    return true;
}

void Font::render_freetype_glyph(std::uint32_t codepoint) const {
    FT_Face face = static_cast<FT_Face>(m_ft_face);
    if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER)) {
        return;
    }

    GlyphBitmap glyph;
    glyph.width = face->glyph->bitmap.width;
    glyph.height = face->glyph->bitmap.rows;
    glyph.x_offset = face->glyph->bitmap_left;
    glyph.y_offset = face->glyph->bitmap_top - static_cast<int>(glyph.height);
    glyph.advance_x = static_cast<std::int32_t>(face->glyph->advance.x >> 6);
    
    glyph.alpha_mask.resize(glyph.width * glyph.height);
    for (std::uint32_t y = 0; y < glyph.height; ++y) {
        for (std::uint32_t x = 0; x < glyph.width; ++x) {
            glyph.alpha_mask[y * glyph.width + x] = face->glyph->bitmap.buffer[y * face->glyph->bitmap.pitch + x];
        }
    }

    m_ft_glyph_cache[codepoint] = std::move(glyph);
}

void Font::render_freetype_glyph_by_index(std::uint32_t glyph_index) const {
    FT_Face face = static_cast<FT_Face>(m_ft_face);
    if (FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER)) {
        return;
    }

    GlyphBitmap glyph;
    glyph.width = face->glyph->bitmap.width;
    glyph.height = face->glyph->bitmap.rows;
    glyph.x_offset = face->glyph->bitmap_left;
    glyph.y_offset = face->glyph->bitmap_top - static_cast<int>(glyph.height);
    glyph.advance_x = static_cast<std::int32_t>(face->glyph->advance.x >> 6);

    glyph.alpha_mask.resize(glyph.width * glyph.height);
    for (std::uint32_t y = 0; y < glyph.height; ++y) {
        for (std::uint32_t x = 0; x < glyph.width; ++x) {
            glyph.alpha_mask[y * glyph.width + x] = face->glyph->bitmap.buffer[y * face->glyph->bitmap.pitch + x];
        }
    }

    m_ft_index_cache[glyph_index] = std::move(glyph);
}

const GlyphBitmap* Font::find_glyph(std::uint32_t codepoint) const {
    if (m_is_freetype) {
        auto it = m_ft_glyph_cache.find(codepoint);
        if (it == m_ft_glyph_cache.end()) {
            render_freetype_glyph(codepoint);
            it = m_ft_glyph_cache.find(codepoint);
        }
        return it != m_ft_glyph_cache.end() ? &it->second : fallback_glyph();
    }

    const auto it = m_glyphs.find(codepoint);
    if (it != m_glyphs.end()) {
        return &it->second;
    }
    return fallback_glyph();
}

const GlyphBitmap* Font::find_glyph_by_index(std::uint32_t glyph_index) const {
    if (!m_is_freetype) {
        return find_glyph(glyph_index);
    }

    auto it = m_ft_index_cache.find(glyph_index);
    if (it == m_ft_index_cache.end()) {
        render_freetype_glyph_by_index(glyph_index);
        it = m_ft_index_cache.find(glyph_index);
    }
    return it != m_ft_index_cache.end() ? &it->second : fallback_glyph();
}

std::uint32_t Font::glyph_index_for_codepoint(std::uint32_t codepoint) const {
    if (!m_is_freetype || m_ft_face == nullptr) {
        return codepoint;
    }

    return static_cast<std::uint32_t>(FT_Get_Char_Index(static_cast<FT_Face>(m_ft_face), codepoint));
}

std::int32_t Font::get_kerning(std::uint32_t left, std::uint32_t right) const {
    if (m_is_freetype) {
        FT_Face face = static_cast<FT_Face>(m_ft_face);
        FT_Vector kerning;
        FT_Get_Kerning(face, FT_Get_Char_Index(face, left), FT_Get_Char_Index(face, right), FT_KERNING_DEFAULT, &kerning);
        return static_cast<std::int32_t>(kerning.x >> 6);
    }

    if (m_kerning_table.empty()) {
        if (left == 'A' && right == 'V') return -2;
        if (left == 'V' && right == 'A') return -2;
        return 0;
    }

    const std::uint64_t key = (static_cast<std::uint64_t>(left) << 32U) | static_cast<std::uint64_t>(right);
    const auto it = m_kerning_table.find(key);
    return it != m_kerning_table.end() ? it->second : 0;
}

const GlyphBitmap* Font::find_scaled_glyph(std::uint32_t codepoint, std::uint32_t scale) const {
    const GlyphBitmap* glyph = find_glyph(codepoint);
    if (glyph == nullptr) {
        return nullptr;
    }

    if (scale <= 1U || m_is_freetype) {
        return glyph;
    }

    return scale_glyph(codepoint, *glyph, scale);
}

const GlyphBitmap* Font::fallback_glyph() const {
    const std::uint32_t cp = static_cast<std::uint32_t>('?');
    if (m_is_freetype) {
        auto it = m_ft_glyph_cache.find(cp);
        if (it == m_ft_glyph_cache.end()) {
            render_freetype_glyph(cp);
            it = m_ft_glyph_cache.find(cp);
        }
        return it != m_ft_glyph_cache.end() ? &it->second : nullptr;
    }

    const auto it = m_glyphs.find(cp);
    if (it != m_glyphs.end()) {
        return &it->second;
    }

    if (!m_glyphs.empty()) {
        return &m_glyphs.begin()->second;
    }

    return nullptr;
}

} // namespace tachyon::text
