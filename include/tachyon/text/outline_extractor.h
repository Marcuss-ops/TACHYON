#pragma once

#include "tachyon/core/scene/evaluated_state.h"
#include <vector>
#include <memory>

namespace tachyon::text {

class Font;

class OutlineExtractor {
public:
    /**
     * Extracts vector contours from a specific glyph index using the provided font.
     * Scale is used to match the layout's pixel size.
     */
    static std::vector<scene::EvaluatedShapePath> extract_glyph_outline(
        const Font& font,
        std::uint32_t glyph_index,
        float scale);
};

} // namespace tachyon::text
