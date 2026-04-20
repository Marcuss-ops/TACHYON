#pragma once

#include <vector>
#include <cstdint>

namespace tachyon {
namespace text {
class Font;
}
}

namespace tachyon::renderer2d::text::shaping {

struct ShapedGlyphRun {
    struct Glyph {
        std::uint32_t codepoint{0};
        std::uint32_t font_glyph_index{0};
        std::int32_t advance_x{0};
        std::int32_t offset_x{0};
        std::int32_t offset_y{0};
    };

    std::vector<Glyph> glyphs;
    std::int32_t width{0};
};

using BitmapFont = tachyon::text::Font; // Use the text module's Font class

ShapedGlyphRun shape_run_with_harfbuzz(const BitmapFont& font, const std::vector<std::uint32_t>& codepoints, std::uint32_t scale);

} // namespace tachyon::renderer2d::text::shaping