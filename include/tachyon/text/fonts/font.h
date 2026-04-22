#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/text/glyph/glyph_bitmap.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace tachyon::text {

struct TextMetrics {
    float ascent{0.0f};
    float descent{0.0f};
    float line_gap{0.0f};
    float baseline{0.0f};
    float underline_y{0.0f};
    float underline_thickness{0.0f};
};

class Font {
public:
    Font();
    ~Font();
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
    Font(Font&& other) noexcept;
    Font& operator=(Font&& other) noexcept;

    bool load_bdf(const std::filesystem::path& path);
    bool load_ttf(const std::filesystem::path& path, std::uint32_t pixel_size);

    const GlyphBitmap* find_glyph(std::uint32_t codepoint) const;
    bool has_glyph(std::uint32_t codepoint) const;
    const GlyphBitmap* find_scaled_glyph(std::uint32_t codepoint, std::uint32_t scale) const;
    const GlyphBitmap* fallback_glyph() const;

    std::int32_t ascent() const { return m_ascent; }
    std::int32_t descent() const { return m_descent; }
    std::int32_t line_height() const { return m_line_height; }
    std::int32_t default_advance() const { return m_default_advance; }
    
    TextMetrics metrics() const {
        return {
            static_cast<float>(m_ascent),
            static_cast<float>(m_descent),
            0.0f, // line_gap is not exposed by the current font loaders
            0.0f, // baseline is derived by the layout engine
            static_cast<float>(m_descent) * 0.5f, // underline position fallback
            1.0f  // underline thickness fallback
        };
    }

    std::int32_t get_kerning(std::uint32_t left, std::uint32_t right) const;
    bool is_loaded() const { return m_loaded; }
    bool has_freetype_face() const { return m_is_freetype && m_ft_face != nullptr; }
    void* freetype_face() const { return m_ft_face; }
    const std::vector<std::uint8_t>& font_data() const { return m_font_data; }
    const GlyphBitmap* find_glyph_by_index(std::uint32_t glyph_index) const;
    std::uint32_t glyph_index_for_codepoint(std::uint32_t codepoint) const;

    std::uint64_t id() const { return m_id; }

private:
    static std::optional<std::uint32_t> parse_hex_row(const std::string& line);
    static std::uint64_t scaled_cache_key(std::uint32_t codepoint, std::uint32_t scale);
    const GlyphBitmap* scale_glyph(std::uint32_t codepoint, const GlyphBitmap& glyph, std::uint32_t scale) const;
    void render_freetype_glyph(std::uint32_t codepoint) const;
    void render_freetype_glyph_by_index(std::uint32_t glyph_index) const;

    std::uint64_t m_id;
    bool m_loaded{false};
    bool m_is_freetype{false};
    std::int32_t m_ascent{0};
    std::int32_t m_descent{0};
    std::int32_t m_line_height{0};
    std::int32_t m_default_advance{0};
    
    // BDF data
    std::unordered_map<std::uint32_t, GlyphBitmap> m_glyphs;
    std::unordered_map<std::uint64_t, std::int32_t> m_kerning_table;
    
    // FreeType data (opaque to avoid public dependency)
    void* m_ft_face{nullptr};
    std::vector<std::uint8_t> m_font_data;
    mutable std::unordered_map<std::uint32_t, GlyphBitmap> m_ft_glyph_cache;
    mutable std::unordered_map<std::uint32_t, GlyphBitmap> m_ft_index_cache;

    mutable std::unordered_map<std::uint64_t, std::unique_ptr<GlyphBitmap>> m_scaled_glyph_cache;
};

using BitmapFont = Font; // For backward compatibility during migration

} // namespace tachyon::text
