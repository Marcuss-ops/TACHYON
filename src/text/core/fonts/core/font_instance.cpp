#include "tachyon/text/fonts/core/font_instance.h"
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

    GlyphBitmap temp_glyph;
    temp_glyph.width = face->glyph->bitmap.width;
    temp_glyph.height = face->glyph->bitmap.rows;
    temp_glyph.x_offset = face->glyph->bitmap_left;
    temp_glyph.y_offset = face->glyph->bitmap_top - static_cast<int>(temp_glyph.height);
    temp_glyph.advance_x = static_cast<std::int32_t>(face->glyph->advance.x >> 6);

    // Allocate from atlas
    std::vector<std::uint8_t> alpha_data(temp_glyph.width * temp_glyph.height);
    for (std::uint32_t y = 0; y < temp_glyph.height; ++y) {
        for (std::uint32_t x = 0; x < temp_glyph.width; ++x) {
            alpha_data[y * temp_glyph.width + x] =
                face->glyph->bitmap.buffer[y * face->glyph->bitmap.pitch + x];
        }
    }

    auto alloc = m_atlas.allocate(temp_glyph.width, temp_glyph.height,
                                   alpha_data.data(), alpha_data.size());
    if (alloc.width == 0 || alloc.height == 0) {
        return; // Allocation failed
    }

    GlyphAtlasEntry entry;
    entry.alloc = alloc;
    entry.x_offset = temp_glyph.x_offset;
    entry.y_offset = temp_glyph.y_offset;
    entry.advance_x = temp_glyph.advance_x;
    m_glyph_cache[codepoint] = std::move(entry);
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

    GlyphBitmap temp_glyph;
    temp_glyph.width = face->glyph->bitmap.width;
    temp_glyph.height = face->glyph->bitmap.rows;
    temp_glyph.x_offset = face->glyph->bitmap_left;
    temp_glyph.y_offset = face->glyph->bitmap_top - static_cast<int>(temp_glyph.height);
    temp_glyph.advance_x = static_cast<std::int32_t>(face->glyph->advance.x >> 6);

    // Allocate from atlas
    std::vector<std::uint8_t> alpha_data(temp_glyph.width * temp_glyph.height);
    for (std::uint32_t y = 0; y < temp_glyph.height; ++y) {
        for (std::uint32_t x = 0; x < temp_glyph.width; ++x) {
            alpha_data[y * temp_glyph.width + x] =
                face->glyph->bitmap.buffer[y * face->glyph->bitmap.pitch + x];
        }
    }

    auto alloc = m_atlas.allocate(temp_glyph.width, temp_glyph.height,
                                   alpha_data.data(), alpha_data.size());
    if (alloc.width == 0 || alloc.height == 0) {
        return; // Allocation failed
    }

    GlyphAtlasEntry entry;
    entry.alloc = alloc;
    entry.x_offset = temp_glyph.x_offset;
    entry.y_offset = temp_glyph.y_offset;
    entry.advance_x = temp_glyph.advance_x;
    m_index_cache[glyph_index] = std::move(entry);
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
    if (it == m_glyph_cache.end()) {
        return nullptr;
    }

    // Fill temp glyph with atlas info
    const auto& entry = it->second;
    m_temp_glyph.width = entry.alloc.width;
    m_temp_glyph.height = entry.alloc.height;
    m_temp_glyph.x_offset = entry.x_offset;
    m_temp_glyph.y_offset = entry.y_offset;
    m_temp_glyph.advance_x = entry.advance_x;
    m_temp_glyph.atlas_data = m_atlas.data().data();
    m_temp_glyph.atlas_x = entry.alloc.x;
    m_temp_glyph.atlas_y = entry.alloc.y;
    m_temp_glyph.atlas_stride = m_atlas.width();

    return &m_temp_glyph;
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
    if (it == m_index_cache.end()) {
        return nullptr;
    }

    // Fill temp glyph with atlas info
    const auto& entry = it->second;
    m_temp_glyph.width = entry.alloc.width;
    m_temp_glyph.height = entry.alloc.height;
    m_temp_glyph.x_offset = entry.x_offset;
    m_temp_glyph.y_offset = entry.y_offset;
    m_temp_glyph.advance_x = entry.advance_x;
    m_temp_glyph.atlas_data = m_atlas.data().data();
    m_temp_glyph.atlas_x = entry.alloc.x;
    m_temp_glyph.atlas_y = entry.alloc.y;
    m_temp_glyph.atlas_stride = m_atlas.width();

    return &m_temp_glyph;
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

