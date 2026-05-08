#include "tachyon/text/animation/text_animator_utils.h"
#include "tachyon/text/layout/layout.h"
#include <algorithm>
#include <cmath>

namespace tachyon::text {

namespace {

std::size_t count_total_clusters(const std::vector<PositionedGlyph>& glyphs) {
    std::size_t total = 0;
    for (const auto& glyph : glyphs) {
        total = std::max(total, glyph.cluster_index + 1U);
    }
    return total;
}

std::size_t count_total_words(const std::vector<PositionedGlyph>& glyphs) {
    std::size_t total = 0;
    for (const auto& glyph : glyphs) {
        total = std::max(total, glyph.word_index + 1U);
    }
    return total;
}

std::size_t count_total_non_space(const std::vector<PositionedGlyph>& glyphs) {
    std::size_t total = 0;
    for (const auto& glyph : glyphs) {
        if (!glyph.whitespace) {
            ++total;
        }
    }
    return total;
}

std::size_t find_line_index(
    const std::vector<TextLine>& lines,
    std::size_t glyph_index) {

    for (std::size_t line_index = 0; line_index < lines.size(); ++line_index) {
        const auto& line = lines[line_index];
        if (glyph_index >= line.glyph_start_index &&
            glyph_index < line.glyph_start_index + line.glyph_count) {
            return line_index;
        }
    }
    return 0;
}

} // namespace

TextAnimatorContext make_text_animator_context(
    const TextLayoutResult& layout,
    std::size_t glyph_index,
    float time) {

    TextAnimatorContext ctx;
    ctx.glyph_index = glyph_index;
    ctx.time = time;
    ctx.total_glyphs = static_cast<float>(layout.glyphs.size());
    ctx.total_clusters = static_cast<float>(count_total_clusters(layout.glyphs));
    ctx.total_words = static_cast<float>(count_total_words(layout.glyphs));
    ctx.total_lines = static_cast<float>(layout.lines.size());
    ctx.total_non_space_glyphs = static_cast<float>(count_total_non_space(layout.glyphs));

    std::size_t non_space_index = 0;
    for (std::size_t i = 0; i < glyph_index && i < layout.glyphs.size(); ++i) {
        if (!layout.glyphs[i].whitespace) {
            ++non_space_index;
        }
    }
    ctx.non_space_glyph_index = static_cast<float>(non_space_index);

    if (glyph_index < layout.glyphs.size()) {
        const auto& glyph = layout.glyphs[glyph_index];
        ctx.cluster_index = glyph.cluster_index;
        ctx.word_index = glyph.word_index;
        ctx.line_index = find_line_index(layout.lines, glyph_index);
        ctx.is_space = glyph.whitespace;
        ctx.is_rtl = glyph.is_rtl;
        ctx.cluster_codepoint_start = glyph.cluster_codepoint_start;
        ctx.cluster_codepoint_count = glyph.cluster_codepoint_count;
    }

    return ctx;
}

} // namespace tachyon::text
