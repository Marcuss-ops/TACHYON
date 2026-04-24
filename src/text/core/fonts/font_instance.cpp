#include "tachyon/text/fonts/font_instance.h"
#include "tachyon/renderer2d/text/freetype/freetype_manager.h"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace tachyon::text {

FontInstance::~FontInstance() = default;

FontInstance::FontInstance(FontInstance&& other) noexcept
    : m_face(other.m_face)
    , m_pixel_size(other.m_pixel_size)
    , m_hinting(other.m_hinting)
    , m_render_mode(other.m_render_mode)
    , m_ascent(other.m_ascent)
    , m_descent(other.m_descent)
    , m_line_height(other.m_line_height)
    , m_default_advance(other.m_default_advance)
    , m_glyph_cache(std::move(other.m_glyph_cache))
    , m_index_cache(std::move(other.m_index_cache)) {
    other.m_face = nullptr;
    other.m_pixel_size = 0;
    other.m_ascent = 0;
    other.m_descent = 0;
    other.m_line_height = 0;
    other.m_default_advance = 0;
}

FontInstance& FontInstance::operator=(FontInstance&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    m_face = other.m_face;
    m_pixel_size = other.m_pixel_size;
    m_hinting = other.m_hinting;
    m_render_mode = other.m_render_mode;
    m_ascent = other.m_ascent;
    m_descent = other.m_descent;
    m_line_height = other.m_line_height;
    m_default_advance = other.m_default_advance;
    m_glyph_cache = std::move(other.m_glyph_cache);
    m_index_cache = std::move(other.m_index_cache);

    other.m_face = nullptr;
    other.m_pixel_size = 0;
    other.m_ascent = 0;
    other.m_descent = 0;
    other.m_line_height = 0;
    other.m_default_advance = 0;

    return *this;
}

bool FontInstance::init(const FontFace& face, std::uint32_t pixel_size,
                       HintingMode hinting, RenderMode render_mode) {
    if (!face.is_loaded() || !face.is_freetype()) {
        return false;
    }

    m_face = &face;
    m_pixel_size = pixel_size;
    m_hinting = hinting;
    m_render_mode = render_mode;

    FT_Face ft_face = static_cast<FT_Face>(face.freetype_face());
    if (!ft_face) {
        return false;
    }

    FT_Set_Pixel_Sizes(ft_face, 0, pixel_size);

    m_ascent = static_cast<std::int32_t>(ft_face->size->metrics.ascender >> 6);
    m_descent = static_cast<std::int32_t>(ft_face->size->metrics.descender >> 6);
    m_line_height = static_cast<std::int32_t>(ft_face->size->metrics.height >> 6);
    m_default_advance = static_cast<std::int32_t>(ft_face->max_advance_width >> 6);

    m_glyph_cache.clear();
    m_index_cache.clear();

    return true;
}

void FontInstance::render_glyph(std::uint32_t codepoint) {
    if (!m_face || !m_face->freetype_face()) {
        return;
    }

    FT_Face face = static_cast<FT_Face>(m_face->freetype_face());

    int load_flags = FT_LOAD_RENDER;
    switch (m_hinting) {
        case HintingMode::None:
            load_flags |= FT_LOAD_NO_HINTING;
            break;
        case HintingMode::Light:
            load_flags |= FT_LOAD_TARGET_LIGHT;
            break;
        case HintingMode::Normal:
            load_flags |= FT_LOAD_TARGET_NORMAL;
            break;
        case HintingMode::Full:
            load_flags |= FT_LOAD_TARGET_MONO;
            break;
    }

    if (FT_Load_Char(face, codepoint, load_flags)) {
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
            glyph.alpha_mask[y * glyph.width + x] =
                face->glyph->bitmap.buffer[y * face->glyph->bitmap.pitch + x];
        }
    }

    m_glyph_cache[codepoint] = std::move(glyph);
}

void FontInstance::render_glyph_by_index(std::uint32_t glyph_index) {
    if (!m_face || !m_face->freetype_face()) {
        return;
    }

    FT_Face face = static_cast<FT_Face>(m_face->freetype_face());

    int load_flags = FT_LOAD_RENDER;
    switch (m_hinting) {
        case HintingMode::None:
            load_flags |= FT_LOAD_NO_HINTING;
            break;
        case HintingMode::Light:
            load_flags |= FT_LOAD_TARGET_LIGHT;
            break;
        case HintingMode::Normal:
            load_flags |= FT_LOAD_TARGET_NORMAL;
            break;
        case HintingMode::Full:
            load_flags |= FT_LOAD_TARGET_MONO;
            break;
    }

    if (FT_Load_Glyph(face, glyph_index, load_flags)) {
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
            glyph.alpha_mask[y * glyph.width + x] =
                face->glyph->bitmap.buffer[y * face->glyph->bitmap.pitch + x];
        }
    }

    m_index_cache[glyph_index] = std::move(glyph);
}

const GlyphBitmap* FontInstance::find_glyph(std::uint32_t codepoint) {
    if (!m_face) {
        return nullptr;
    }

    auto it = m_glyph_cache.find(codepoint);
    if (it == m_glyph_cache.end()) {
        render_glyph(codepoint);
        it = m_glyph_cache.find(codepoint);
    }
    return it != m_glyph_cache.end() ? &it->second : nullptr;
}

const GlyphBitmap* FontInstance::find_glyph_by_index(std::uint32_t glyph_index) {
    if (!m_face) {
        return nullptr;
    }

    auto it = m_index_cache.find(glyph_index);
    if (it == m_index_cache.end()) {
        render_glyph_by_index(glyph_index);
        it = m_index_cache.find(glyph_index);
    }
    return it != m_index_cache.end() ? &it->second : nullptr;
}

std::int32_t FontInstance::get_kerning(std::uint32_t left, std::uint32_t right) const {
    if (!m_face || !m_face->freetype_face()) {
        return 0;
    }

    FT_Face face = static_cast<FT_Face>(m_face->freetype_face());
    FT_Vector kerning;
    FT_Get_Kerning(face,
                   FT_Get_Char_Index(face, left),
                   FT_Get_Char_Index(face, right),
                   FT_KERNING_DEFAULT,
                   &kerning);
    return static_cast<std::int32_t>(kerning.x >> 6);
}

FontInstanceKey FontInstance::key() const {
    return {
        m_face ? m_face->id() : 0,
        m_pixel_size,
        m_hinting,
        m_render_mode
    };
}

} // namespace tachyon::text
