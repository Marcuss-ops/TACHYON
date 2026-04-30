#pragma once

#include "tachyon/text/fonts/font_face.h"
#include "tachyon/renderer2d/text/glyph/glyph_bitmap.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace tachyon::text {

enum class HintingMode : std::uint8_t {
    None,
    Light,
    Normal,
    Full
};

enum class RenderMode : std::uint8_t {
    Normal,
    Light,
    Mono,
    LCD
};

struct FontInstanceKey {
    std::uint64_t face_id;
    std::uint32_t pixel_size;
    HintingMode hinting;
    RenderMode render_mode;

    bool operator==(const FontInstanceKey& other) const {
        return face_id == other.face_id &&
               pixel_size == other.pixel_size &&
               hinting == other.hinting &&
               render_mode == other.render_mode;
    }
};

struct FontInstanceKeyHash {
    std::size_t operator()(const FontInstanceKey& key) const {
        std::size_t h = 0;
        h ^= std::hash<std::uint64_t>{}(key.face_id) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<std::uint32_t>{}(key.pixel_size) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<std::uint8_t>{}(static_cast<std::uint8_t>(key.hinting)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<std::uint8_t>{}(static_cast<std::uint8_t>(key.render_mode)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

class FontInstance {
public:
    FontInstance() = default;
    ~FontInstance();

    FontInstance(const FontInstance&) = delete;
    FontInstance& operator=(const FontInstance&) = delete;
    FontInstance(FontInstance&& other) noexcept;
    FontInstance& operator=(FontInstance&& other) noexcept;

    bool init(const FontFace& face, std::uint32_t pixel_size,
              HintingMode hinting = HintingMode::Normal,
              RenderMode render_mode = RenderMode::Normal);

    const GlyphBitmap* find_glyph(std::uint32_t codepoint);
    const GlyphBitmap* find_glyph_by_index(std::uint32_t glyph_index);

    std::int32_t get_kerning(std::uint32_t left, std::uint32_t right) const;

    std::int32_t ascent() const { return m_ascent; }
    std::int32_t descent() const { return m_descent; }
    std::int32_t line_height() const { return m_line_height; }
    std::int32_t default_advance() const { return m_default_advance; }

    std::uint32_t pixel_size() const { return m_pixel_size; }
    HintingMode hinting() const { return m_hinting; }
    RenderMode render_mode() const { return m_render_mode; }

    const FontFace* face() const { return m_face; }
    FontInstanceKey key() const;

private:
    void render_glyph(std::uint32_t codepoint);
    void render_glyph_by_index(std::uint32_t glyph_index);

    const FontFace* m_face{nullptr};
    std::uint32_t m_pixel_size{0};
    HintingMode m_hinting{HintingMode::Normal};
    RenderMode m_render_mode{RenderMode::Normal};

    std::int32_t m_ascent{0};
    std::int32_t m_descent{0};
    std::int32_t m_line_height{0};
    std::int32_t m_default_advance{0};

    mutable std::unordered_map<std::uint32_t, GlyphBitmap> m_glyph_cache;
    mutable std::unordered_map<std::uint32_t, GlyphBitmap> m_index_cache;
};

} // namespace tachyon::text
