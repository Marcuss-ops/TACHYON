#include "tachyon/text/fonts/font_face.h"
#include "tachyon/renderer2d/text/freetype/freetype_manager.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <fstream>
#include <istream>
#include <iterator>
#include <atomic>
#include <functional>

namespace tachyon::text {

std::atomic<std::uint64_t> FontFace::next_font_id{1};

namespace {

std::uint64_t compute_hash(const std::vector<std::uint8_t>& data) {
    std::uint64_t hash = 0;
    for (std::size_t i = 0; i < data.size(); ++i) {
        hash = hash * 31 + static_cast<std::uint64_t>(data[i]);
    }
    return hash;
}

} // anonymous namespace

std::uint64_t FontFace::compute_content_hash(const std::vector<std::uint8_t>& data) {
    return compute_hash(data);
}

FontFace::FontFace()
    : m_id(next_font_id.fetch_add(1, std::memory_order_relaxed))
    , m_content_hash(0)
    , m_loaded(false)
    , m_is_freetype(false)
    , m_ascent(0)
    , m_descent(0)
    , m_line_height(0)
    , m_default_advance(0)
    , m_ft_face(nullptr) {}

FontFace::~FontFace() {
    if (m_ft_face) {
        FT_Done_Face(static_cast<FT_Face>(m_ft_face));
    }
}

FontFace::FontFace(FontFace&& other) noexcept
    : m_id(other.m_id)
    , m_content_hash(other.m_content_hash)
    , m_loaded(other.m_loaded)
    , m_is_freetype(other.m_is_freetype)
    , m_info(std::move(other.m_info))
    , m_ascent(other.m_ascent)
    , m_descent(other.m_descent)
    , m_line_height(other.m_line_height)
    , m_default_advance(other.m_default_advance)
    , m_ft_face(other.m_ft_face)
    , m_font_data(std::move(other.m_font_data)) {
    other.m_loaded = false;
    other.m_is_freetype = false;
    other.m_ascent = 0;
    other.m_descent = 0;
    other.m_line_height = 0;
    other.m_default_advance = 0;
    other.m_ft_face = nullptr;
}

FontFace& FontFace::operator=(FontFace&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (m_ft_face) {
        FT_Done_Face(static_cast<FT_Face>(m_ft_face));
        m_ft_face = nullptr;
    }

    m_id = other.m_id;
    m_content_hash = other.m_content_hash;
    m_loaded = other.m_loaded;
    m_is_freetype = other.m_is_freetype;
    m_info = std::move(other.m_info);
    m_ascent = other.m_ascent;
    m_descent = other.m_descent;
    m_line_height = other.m_line_height;
    m_default_advance = other.m_default_advance;
    m_ft_face = other.m_ft_face;
    m_font_data = std::move(other.m_font_data);

    other.m_loaded = false;
    other.m_is_freetype = false;
    other.m_ascent = 0;
    other.m_descent = 0;
    other.m_line_height = 0;
    other.m_default_advance = 0;
    other.m_ft_face = nullptr;

    return *this;
}

bool FontFace::load_from_file(const std::filesystem::path& path) {
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
    m_font_data.clear();
    m_info.path = path;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    m_font_data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    if (m_font_data.empty()) {
        return false;
    }

    m_content_hash = compute_content_hash(m_font_data);

    FT_Face face;
    if (FT_New_Memory_Face(::tachyon::renderer2d::text::get_ft_library(),
                            m_font_data.data(),
                            static_cast<FT_Long>(m_font_data.size()),
                            0,
                            &face)) {
        m_font_data.clear();
        return false;
    }

    m_is_freetype = true;
    m_ft_face = face;

    if (m_info.family.empty()) {
        if (face->family_name) {
            m_info.family = face->family_name;
        }
    }

    m_ascent = static_cast<std::int32_t>(face->size->metrics.ascender >> 6);
    m_descent = static_cast<std::int32_t>(face->size->metrics.descender >> 6);
    m_line_height = static_cast<std::int32_t>(face->size->metrics.height >> 6);
    m_default_advance = static_cast<std::int32_t>(face->max_advance_width >> 6);

    if (m_info.metrics_override.has_value()) {
        if (m_info.metrics_override->ascent.has_value()) {
            m_ascent = static_cast<std::int32_t>(*m_info.metrics_override->ascent);
        }
        if (m_info.metrics_override->descent.has_value()) {
            m_descent = static_cast<std::int32_t>(*m_info.metrics_override->descent);
        }
    }

    m_loaded = true;
    return true;
}

bool FontFace::has_glyph(std::uint32_t codepoint) const {
    if (!m_is_freetype || m_ft_face == nullptr) {
        return false;
    }
    return FT_Get_Char_Index(static_cast<FT_Face>(m_ft_face), codepoint) != 0;
}

std::uint32_t FontFace::glyph_index_for_codepoint(std::uint32_t codepoint) const {
    if (!m_is_freetype || m_ft_face == nullptr) {
        return codepoint;
    }
    return static_cast<std::uint32_t>(FT_Get_Char_Index(static_cast<FT_Face>(m_ft_face), codepoint));
}

std::int32_t FontFace::get_kerning(std::uint32_t left, std::uint32_t right) const {
    if (!m_is_freetype || m_ft_face == nullptr) {
        return 0;
    }

    FT_Face face = static_cast<FT_Face>(m_ft_face);
    FT_Vector kerning;
    FT_Get_Kerning(face,
                   FT_Get_Char_Index(face, left),
                   FT_Get_Char_Index(face, right),
                   FT_KERNING_DEFAULT,
                   &kerning);
    return static_cast<std::int32_t>(kerning.x >> 6);
}

void FontFace::collect_codepoints(std::unordered_set<std::uint32_t>& codepoints) const {
    if (!m_is_freetype || m_ft_face == nullptr) {
        return;
    }

    FT_Face face = static_cast<FT_Face>(m_ft_face);
    FT_UInt glyph_index;
    FT_ULong charcode = FT_Get_First_Char(face, &glyph_index);
    while (glyph_index != 0) {
        codepoints.insert(static_cast<std::uint32_t>(charcode));
        charcode = FT_Get_Next_Char(face, charcode, &glyph_index);
    }
}

} // namespace tachyon::text
