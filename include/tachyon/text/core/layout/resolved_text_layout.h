#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace tachyon::math {

struct RectF {
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};
};

} // namespace tachyon::math

namespace tachyon::text {

class Font; // Forward declaration

/**
 * @brief Represents a logical cluster of characters (e.g. a base character + combining marks, or a ligature).
 * Emitted by the HarfBuzz shaping pass.
 */
struct GlyphCluster {
    std::size_t source_text_start;
    std::size_t source_text_length;
    std::size_t glyph_start;
    std::size_t glyph_count;
};

/**
 * @brief A shaped and positioned glyph, ready for rasterization or animation.
 */
struct ResolvedGlyph {
    std::uint32_t codepoint;
    std::uint32_t glyph_index;
    
    // Position relative to the layout origin (baseline start)
    ::tachyon::math::Vector2 position;
    
    // The advance width of this glyph (for determining next glyph position)
    float advance_x;
    float advance_y;
    
    // Which font resolved this glyph (important for fallback fonts)
    const Font* font{nullptr};
    
    // Reference to the logical cluster
    std::size_t cluster_index;
    
    // Styling attributes (these can be animated/overridden per glyph)
    float font_size;
    ::tachyon::ColorSpec fill_color;
    ::tachyon::ColorSpec stroke_color;
    float stroke_width;
    float opacity{1.0f};
    ::tachyon::math::Vector2 scale{1.0f, 1.0f};
    float rotation{0.0f}; // in degrees
    bool is_rtl{false};
    
    // Bounding box of the glyph (relative to its position)
    ::tachyon::math::RectF bounds;
};

/**
 * @brief A contiguous run of glyphs sharing the same font, size, and styling spans.
 */
struct TextRun {
    std::size_t start_glyph_index;
    std::size_t length;
    const Font* font{nullptr};
    float font_size;
    
    ::tachyon::math::RectF bounds;
};

/**
 * @brief A line of text, resulting from word-wrapping or hard breaks.
 */
struct TextLine {
    std::size_t start_glyph_index;
    std::size_t length;
    
    float baseline_y;
    float ascent;
    float descent;
    
    ::tachyon::math::RectF bounds;
};

/**
 * @brief A paragraph of text, separated by explicit newline characters.
 */
struct Paragraph {
    std::size_t start_line_index;
    std::size_t line_count;
    
    ::tachyon::math::RectF bounds;
};

/**
 * @brief The final output contract of the TextLayoutEngine.
 * This struct is strictly deterministic and contains everything needed
 * by both the Text Animator system and the Rasterizer.
 */
struct ResolvedTextLayout {
    std::vector<ResolvedGlyph> glyphs;
    std::vector<GlyphCluster> clusters;
    std::vector<TextRun> runs;
    std::vector<TextLine> lines;
    std::vector<Paragraph> paragraphs;
    
    // Overall bounding box of the entire layout
    ::tachyon::math::RectF total_bounds;
    
    // Text-on-path data (if applicable)
    bool is_on_path{false};
    
    // Helper to get bounding box for a logical character range
    ::tachyon::math::RectF get_range_bounds(std::size_t start_char_index, std::size_t end_char_index) const;
};

} // namespace tachyon::text
