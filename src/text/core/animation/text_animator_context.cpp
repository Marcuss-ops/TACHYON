#include "tachyon/text/animation/text_animator_utils.h"
#include <algorithm>
#include <cmath>

namespace tachyon::text {

namespace {

// For ResolvedGlyph + ResolvedTextLine (used by ResolvedTextLayout)
std::size_t count_total_words_rg(const std::vector<ResolvedGlyph>& glyphs) {
    std::size_t total = 0;
    for (const auto& glyph : glyphs) {
        total = std::max(total, glyph.word_index + 1U);
    }
    return total;
}

std::size_t count_total_non_space_rg(const std::vector<ResolvedGlyph>& glyphs) {
    std::size_t total = 0;
    for (const auto& glyph : glyphs) {
        if (!glyph.is_space && !glyph.whitespace) {
            ++total;
        }
    }
    return total;
}

std::size_t find_line_index_rtl(
    const std::vector<ResolvedTextLine>& lines,
    std::size_t glyph_index) {

    for (std::size_t line_index = 0; line_index < lines.size(); ++line_index) {
        const auto& line = lines[line_index];
        if (glyph_index >= line.start_glyph_index &&
            glyph_index < line.start_glyph_index + line.length) {
            return line_index;
        }
    }
    return 0;
}

TextAnimatorContext build_context_resolved(
    std::size_t glyph_index,
    float time,
    const std::vector<ResolvedGlyph>& glyphs,
    const std::vector<ResolvedTextLine>& lines) {

    TextAnimatorContext ctx;
    ctx.glyph_index = glyph_index;
    ctx.time = time;
    ctx.total_glyphs = static_cast<float>(glyphs.size());
    std::size_t max_cluster = 0;
    for (const auto& glyph : glyphs) {
        max_cluster = std::max(max_cluster, glyph.cluster_index + 1U);
    }
    ctx.total_clusters = static_cast<float>(max_cluster);
    ctx.total_words = static_cast<float>(count_total_words_rg(glyphs));
    ctx.total_lines = static_cast<float>(lines.size());
    ctx.total_non_space_glyphs = static_cast<float>(count_total_non_space_rg(glyphs));

    std::size_t non_space_index = 0;
    for (std::size_t i = 0; i < glyph_index && i < glyphs.size(); ++i) {
        if (!glyphs[i].is_space && !glyphs[i].whitespace) {
            ++non_space_index;
        }
    }
    ctx.non_space_glyph_index = static_cast<float>(non_space_index);

    if (glyph_index < glyphs.size()) {
        const auto& glyph = glyphs[glyph_index];
        ctx.cluster_index = glyph.cluster_index;
        ctx.word_index = glyph.word_index;
        ctx.line_index = 0; // Not used for resolved glyphs
        ctx.is_space = glyph.is_space || glyph.whitespace;
        ctx.is_rtl = glyph.is_rtl;
    }

    return ctx;
}

// For PositionedGlyph + TextLine (used by TextLayoutResult)
std::size_t count_total_words_pg(const std::vector<PositionedGlyph>& glyphs) {
    std::size_t total = 0;
    for (const auto& glyph : glyphs) {
        total = std::max(total, glyph.word_index + 1U);
    }
    return total;
}

std::size_t count_total_non_space_pg(const std::vector<PositionedGlyph>& glyphs) {
    std::size_t total = 0;
    for (const auto& glyph : glyphs) {
        if (!glyph.whitespace) {
            ++total;
        }
    }
    return total;
}

std::size_t find_line_index_tl(
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

TextAnimatorContext build_context_positioned(
    std::size_t glyph_index,
    float time,
    const std::vector<PositionedGlyph>& glyphs,
    const std::vector<TextLine>& lines) {

    TextAnimatorContext ctx;
    ctx.glyph_index = glyph_index;
    ctx.time = time;
    ctx.total_glyphs = static_cast<float>(glyphs.size());
    std::size_t max_cluster = 0;
    for (const auto& glyph : glyphs) {
        max_cluster = std::max(max_cluster, glyph.cluster_index + 1U);
    }
    ctx.total_clusters = static_cast<float>(max_cluster);
    ctx.total_words = static_cast<float>(count_total_words_pg(glyphs));
    ctx.total_lines = static_cast<float>(lines.size());
    ctx.total_non_space_glyphs = static_cast<float>(count_total_non_space_pg(glyphs));

    std::size_t non_space_index = 0;
    for (std::size_t i = 0; i < glyph_index && i < glyphs.size(); ++i) {
        if (!glyphs[i].whitespace) {
            ++non_space_index;
        }
    }
    ctx.non_space_glyph_index = static_cast<float>(non_space_index);

    if (glyph_index < glyphs.size()) {
        const auto& glyph = glyphs[glyph_index];
        ctx.cluster_index = glyph.cluster_index;
        ctx.word_index = glyph.word_index;
        ctx.line_index = find_line_index_tl(lines, glyph_index);
        ctx.is_space = glyph.whitespace;
        ctx.is_rtl = glyph.is_rtl;
        ctx.cluster_codepoint_start = glyph.cluster_codepoint_start;
        ctx.cluster_codepoint_count = glyph.cluster_codepoint_count;
    }

    return ctx;
}

} // namespace

TextAnimatorContext make_text_animator_context(
    const ResolvedTextLayout& layout,
    std::size_t glyph_index,
    float time) {
    TextAnimatorContext ctx = build_context_resolved(glyph_index, time, layout.glyphs, layout.lines);
    if (ctx.cluster_index < layout.clusters.size()) {
        const auto& cluster = layout.clusters[ctx.cluster_index];
        ctx.cluster_codepoint_start = cluster.source_text_start;
        ctx.cluster_codepoint_count = cluster.source_text_length;
    }
    return ctx;
}

TextAnimatorContext make_text_animator_context(
    const TextLayoutResult& layout,
    std::size_t glyph_index,
    float time) {
    return build_context_positioned(glyph_index, time, layout.glyphs, layout.lines);
}

} // namespace tachyon::text
