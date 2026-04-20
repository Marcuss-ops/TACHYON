#pragma once

#include "tachyon/renderer2d/text/glyph/glyph_bitmap.h"
#include <filesystem>
#include <unordered_map>
#include <optional>

namespace tachyon::renderer2d::text {

struct FontMetrics {
    std::int32_t ascent{0};
    std::int32_t descent{0};
    std::int32_t line_height{0};
    std::int32_t default_advance{0};
};

struct LoadedFont {
    FontMetrics metrics;
    std::unordered_map<std::uint32_t, GlyphBitmap> glyphs;
    std::unordered_map<std::uint64_t, std::int32_t> kerning_table;
    bool success{false};
};

class GlyphLoader {
public:
    static LoadedFont load_bdf(const std::filesystem::path& path);
};

} // namespace tachyon::renderer2d::text
