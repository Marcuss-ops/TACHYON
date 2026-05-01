#include "tachyon/text/fonts/core/font.h"

#include "tachyon/renderer2d/text/freetype/freetype_manager.h"
#include "tachyon/renderer2d/text/glyph/glyph_loader.h"
#include "tachyon/renderer2d/text/utils/font_utils.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace tachyon::text {
namespace {

std::atomic<std::uint64_t> next_font_id{1};

template <typename T>
T clamp_scale(T value) {
    return value;
}

GlyphBitmap make_fallback_glyph() {
    GlyphBitmap glyph;
    glyph.width = 1;
    glyph.height = 1;
    glyph.advance_x = 1;
    glyph.alpha_mask = {255};
    return glyph;
}

} // namespace

Font::Font()
    : m_id(next_font_id.fetch_add(1, std::memory_order_relaxed)) {}

Font::~Font() {
    if (m_ft_face) {
        FT_Done_Face(static_cast<FT_Face>(m_ft_face));
        m_ft_face = nullptr;
    }
}

Font::Font(Font&& other) noexcept
    : m_id(other.m_id),
      m_loaded(other.m_loaded),
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
      m_ft_sdf_cache(std::move(other.m_ft_sdf_cache)),
      m_ft_sdf_index_cache(std::move(other.m_ft_sdf_index_cache)),
      m_scaled_glyph_cache(std::move(other.m_scaled_glyph_cache)) {
    other.m_ft_face = nullptr;
    other.m_loaded = false;
    other.m_is_freetype = false;
    other.m_ascent = 0;
    other.m_descent = 0;
    other.m_line_height = 0;
    other.m_default_advance = 0;
}

Font& Font::operator=(Font&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (m_ft_face) {
        FT_Done_Face(static_cast<FT_Face>(m_ft_face));
        m_ft_face = nullptr;
    }

    m_id = other.m_id;
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
    m_ft_sdf_cache = std::move(other.m_ft_sdf_cache);
    m_ft_sdf_index_cache = std::move(other.m_ft_sdf_index_cache);
    m_scaled_glyph_cache = std::move(other.m_scaled_glyph_cache);

    other.m_ft_face = nullptr;
    other.m_loaded = false;
    other.m_is_freetype = false;
    other.m_ascent = 0;
    other.m_descent = 0;
    other.m_line_height = 0;
    other.m_default_advance = 0;
    return *this;
}

std::optional<std::uint32_t> Font::parse_hex_row(const std::string& line) {
    const auto cleaned = renderer2d::text::trim_copy(line);
    if (cleaned.empty()) {
        return std::nullopt;
    }

    std::uint32_t value = 0;
    for (char ch : cleaned) {
        value <<= 4U;
        if (ch >= '0' && ch <= '9') value |= static_cast<std::uint32_t>(ch - '0');
        else if (ch >= 'a' && ch <= 'f') value |= static_cast<std::uint32_t>(10 + ch - 'a');
        else if (ch >= 'A' && ch <= 'F') value |= static_cast<std::uint32_t>(10 + ch - 'A');
        else return std::nullopt;
    }
    return value;
}

std::uint64_t Font::scaled_cache_key(std::uint32_t codepoint, std::uint32_t scale) {
    return (static_cast<std::uint64_t>(codepoint) << 32U) | static_cast<std::uint64_t>(scale);
}

const GlyphBitmap* Font::scale_glyph(std::uint32_t codepoint, const GlyphBitmap& glyph, std::uint32_t scale) const {
    if (scale <= 1U || glyph.width == 0U || glyph.height == 0U) {
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

    const GlyphBitmap* result = scaled.get();
    m_scaled_glyph_cache.emplace(key, std::move(scaled));
    return result;
}

bool Font::load_bdf(const std::filesystem::path& path) {
    renderer2d::text::LoadedFont loaded = renderer2d::text::GlyphLoader::load_bdf(path);
    if (!loaded.success) {
        return false;
    }

    if (m_ft_face) {
        FT_Done_Face(static_cast<FT_Face>(m_ft_face));
        m_ft_face = nullptr;
    }

    m_loaded = true;
    m_is_freetype = false;
    m_ascent = loaded.metrics.ascent;
    m_descent = loaded.metrics.descent;
    m_line_height = loaded.metrics.line_height;
    m_default_advance = loaded.metrics.default_advance;
    m_glyphs = std::move(loaded.glyphs);
    m_kerning_table = std::move(loaded.kerning_table);
    m_font_data.clear();
    m_ft_glyph_cache.clear();
    m_ft_index_cache.clear();
    m_ft_sdf_cache.clear();
    m_ft_sdf_index_cache.clear();
    m_scaled_glyph_cache.clear();
    return true;
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
    m_ft_sdf_cache.clear();
    m_ft_sdf_index_cache.clear();
    m_font_data.clear();

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    m_font_data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    if (m_font_data.empty()) {
        return false;
    }

    FT_Face face = nullptr;
    if (FT_New_Memory_Face(renderer2d::text::get_ft_library(),
                           m_font_data.data(),
                           static_cast<FT_Long>(m_font_data.size()),
                           0,
                           &face) != 0) {
        m_font_data.clear();
        return false;
    }

    if (FT_Set_Pixel_Sizes(face, 0, pixel_size) != 0) {
        FT_Done_Face(face);
        m_font_data.clear();
        return false;
    }

    m_ft_face = face;
    m_is_freetype = true;
    m_loaded = true;
    m_ascent = static_cast<std::int32_t>(face->size->metrics.ascender >> 6);
    m_descent = static_cast<std::int32_t>(face->size->metrics.descender >> 6);
    m_line_height = static_cast<std::int32_t>(face->size->metrics.height >> 6);
    m_default_advance = static_cast<std::int32_t>(face->max_advance_width >> 6);
    return true;
}

void Font::render_freetype_glyph(std::uint32_t codepoint) const {
    if (!m_ft_face) {
        return;
    }

    FT_Face face = static_cast<FT_Face>(m_ft_face);
    if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER) != 0) {
        return;
    }

    GlyphBitmap glyph;
    glyph.width = face->glyph->bitmap.width;
    glyph.height = face->glyph->bitmap.rows;
    glyph.x_offset = face->glyph->bitmap_left;
    glyph.y_offset = face->glyph->bitmap_top - static_cast<std::int32_t>(glyph.height);
    glyph.advance_x = static_cast<std::int32_t>(face->glyph->advance.x >> 6);
    glyph.alpha_mask.resize(static_cast<std::size_t>(glyph.width) * static_cast<std::size_t>(glyph.height), 0U);

    const int pitch = std::abs(face->glyph->bitmap.pitch);
    const unsigned char* buffer = face->glyph->bitmap.buffer;
    for (std::uint32_t y = 0; y < glyph.height; ++y) {
        for (std::uint32_t x = 0; x < glyph.width; ++x) {
            glyph.alpha_mask[y * glyph.width + x] = buffer[y * pitch + x];
        }
    }

    m_ft_glyph_cache[codepoint] = std::move(glyph);
}

void Font::render_freetype_glyph_by_index(std::uint32_t glyph_index) const {
    if (!m_ft_face) {
        return;
    }

    FT_Face face = static_cast<FT_Face>(m_ft_face);
    if (FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER) != 0) {
        return;
    }

    GlyphBitmap glyph;
    glyph.width = face->glyph->bitmap.width;
    glyph.height = face->glyph->bitmap.rows;
    glyph.x_offset = face->glyph->bitmap_left;
    glyph.y_offset = face->glyph->bitmap_top - static_cast<std::int32_t>(glyph.height);
    glyph.advance_x = static_cast<std::int32_t>(face->glyph->advance.x >> 6);
    glyph.alpha_mask.resize(static_cast<std::size_t>(glyph.width) * static_cast<std::size_t>(glyph.height), 0U);

    const int pitch = std::abs(face->glyph->bitmap.pitch);
    const unsigned char* buffer = face->glyph->bitmap.buffer;
    for (std::uint32_t y = 0; y < glyph.height; ++y) {
        for (std::uint32_t x = 0; x < glyph.width; ++x) {
            glyph.alpha_mask[y * glyph.width + x] = buffer[y * pitch + x];
        }
    }

    m_ft_index_cache[glyph_index] = std::move(glyph);
}

void Font::render_freetype_sdf(std::uint32_t codepoint) const {
    render_freetype_glyph(codepoint);
    auto it = m_ft_glyph_cache.find(codepoint);
    if (it != m_ft_glyph_cache.end()) {
        m_ft_sdf_cache[codepoint] = it->second;
    }
}

void Font::render_freetype_sdf_by_index(std::uint32_t glyph_index) const {
    render_freetype_glyph_by_index(glyph_index);
    auto it = m_ft_index_cache.find(glyph_index);
    if (it != m_ft_index_cache.end()) {
        m_ft_sdf_index_cache[glyph_index] = it->second;
    }
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

const GlyphBitmap* Font::find_sdf_glyph(std::uint32_t codepoint) const {
    if (m_is_freetype) {
        auto it = m_ft_sdf_cache.find(codepoint);
        if (it == m_ft_sdf_cache.end()) {
            render_freetype_sdf(codepoint);
            it = m_ft_sdf_cache.find(codepoint);
        }
        return it != m_ft_sdf_cache.end() ? &it->second : fallback_glyph();
    }
    return find_glyph(codepoint);
}

bool Font::has_glyph(std::uint32_t codepoint) const {
    if (m_is_freetype) {
        return m_ft_face != nullptr && FT_Get_Char_Index(static_cast<FT_Face>(m_ft_face), codepoint) != 0;
    }
    return m_glyphs.find(codepoint) != m_glyphs.end();
}

const GlyphBitmap* Font::find_scaled_glyph(std::uint32_t codepoint, std::uint32_t scale) const {
    const GlyphBitmap* glyph = find_glyph(codepoint);
    if (glyph == nullptr) {
        return nullptr;
    }
    if (m_is_freetype || scale <= 1U) {
        return glyph;
    }
    return scale_glyph(codepoint, *glyph, scale);
}

const GlyphBitmap* Font::fallback_glyph() const {
    const auto it = m_glyphs.find(static_cast<std::uint32_t>('?'));
    if (it != m_glyphs.end()) {
        return &it->second;
    }

    if (!m_is_freetype) {
        return !m_glyphs.empty() ? &m_glyphs.begin()->second : nullptr;
    }

    auto cache_it = m_ft_glyph_cache.find(static_cast<std::uint32_t>('?'));
    if (cache_it == m_ft_glyph_cache.end()) {
        render_freetype_glyph(static_cast<std::uint32_t>('?'));
        cache_it = m_ft_glyph_cache.find(static_cast<std::uint32_t>('?'));
    }
    return cache_it != m_ft_glyph_cache.end() ? &cache_it->second : nullptr;
}

const GlyphBitmap* Font::find_glyph_by_index(std::uint32_t glyph_index) const {
    if (!m_is_freetype) {
        return nullptr;
    }

    auto it = m_ft_index_cache.find(glyph_index);
    if (it == m_ft_index_cache.end()) {
        render_freetype_glyph_by_index(glyph_index);
        it = m_ft_index_cache.find(glyph_index);
    }
    return it != m_ft_index_cache.end() ? &it->second : fallback_glyph();
}

const GlyphBitmap* Font::find_sdf_glyph_by_index(std::uint32_t glyph_index) const {
    if (!m_is_freetype) {
        return nullptr;
    }

    auto it = m_ft_sdf_index_cache.find(glyph_index);
    if (it == m_ft_sdf_index_cache.end()) {
        render_freetype_sdf_by_index(glyph_index);
        it = m_ft_sdf_index_cache.find(glyph_index);
    }
    return it != m_ft_sdf_index_cache.end() ? &it->second : fallback_glyph();
}

std::uint32_t Font::glyph_index_for_codepoint(std::uint32_t codepoint) const {
    if (!m_is_freetype || m_ft_face == nullptr) {
        return codepoint;
    }
    return static_cast<std::uint32_t>(FT_Get_Char_Index(static_cast<FT_Face>(m_ft_face), codepoint));
}

std::int32_t Font::get_kerning(std::uint32_t left, std::uint32_t right) const {
    if (m_is_freetype && m_ft_face != nullptr) {
        FT_Face face = static_cast<FT_Face>(m_ft_face);
        FT_Vector kerning{};
        FT_Get_Kerning(face, FT_Get_Char_Index(face, left), FT_Get_Char_Index(face, right), FT_KERNING_DEFAULT, &kerning);
        return static_cast<std::int32_t>(kerning.x >> 6);
    }

    const std::uint64_t key = (static_cast<std::uint64_t>(left) << 32U) | static_cast<std::uint64_t>(right);
    const auto it = m_kerning_table.find(key);
    return it != m_kerning_table.end() ? it->second : 0;
}

} // namespace tachyon::text
