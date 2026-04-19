#pragma once

#include "tachyon/renderer2d/framebuffer.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace tachyon::text {

struct GlyphBitmap {
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::int32_t x_offset{0};
    std::int32_t y_offset{0};
    std::int32_t advance_x{0};
    std::vector<std::uint8_t> alpha_mask;
};

class BitmapFont {
public:
    bool load_bdf(const std::filesystem::path& path);

    const GlyphBitmap* find_glyph(std::uint32_t codepoint) const;
    const GlyphBitmap* find_scaled_glyph(std::uint32_t codepoint, std::uint32_t scale) const;
    const GlyphBitmap* fallback_glyph() const;

    std::int32_t ascent() const { return m_ascent; }
    std::int32_t descent() const { return m_descent; }
    std::int32_t line_height() const { return m_line_height; }
    std::int32_t default_advance() const { return m_default_advance; }
    bool is_loaded() const { return m_loaded; }

private:
    static std::optional<std::uint32_t> parse_hex_row(const std::string& line);
    static std::uint64_t scaled_cache_key(std::uint32_t codepoint, std::uint32_t scale);
    const GlyphBitmap* scale_glyph(std::uint32_t codepoint, const GlyphBitmap& glyph, std::uint32_t scale) const;

    bool m_loaded{false};
    std::int32_t m_ascent{0};
    std::int32_t m_descent{0};
    std::int32_t m_line_height{0};
    std::int32_t m_default_advance{0};
    std::unordered_map<std::uint32_t, GlyphBitmap> m_glyphs;
    mutable std::unordered_map<std::uint64_t, std::shared_ptr<GlyphBitmap>> m_scaled_glyph_cache;
};

} // namespace tachyon::text
