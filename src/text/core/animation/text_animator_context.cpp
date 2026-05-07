#include "tachyon/text/animation/text_animator_utils.h"
#include <algorithm>
#include <cmath>

namespace tachyon::text {

namespace {

template <typename GlyphT>
std::size_t count_total_clusters(const std::vector<GlyphT>& glyphs) {
    std::size_t total = 0;
    for (const auto& glyph : glyphs) {
        total = std::max(total, glyph.cluster_index + 1U);
    }
    return total;
}

template <typename GlyphT>
std::size_t count_total_words(const std::vector<GlyphT>& glyphs) {
    std::size_t total = 0;
    for (const auto& glyph : glyphs) {
        total = std::max(total, glyph.word_index + 1U);
    }
    return total;
}

template <typename GlyphT, typename IsSpaceFn>
std::size_t count_total_non_space(const std::vector<GlyphT>& glyphs, IsSpaceFn&& is_space) {
    std::size_t total = 0;
    for (const auto& glyph : glyphs) {
        if (!is_space(glyph)) {
            ++total;
        }
    }
    return total;
}

template <typename GlyphT, typename LineT, typename IsSpaceFn, typename FindLineFn, typename PopulateExtraFn>
TextAnimatorContext build_context_common(
    std::size_t glyph_index,
    float time,
    const std::vector<GlyphT>& glyphs,
    const std::vector<LineT>& lines,
    IsSpaceFn&& is_space,
    FindLineFn&& find_line,
    PopulateExtraFn&& populate_extra) {

    TextAnimatorContext ctx;
    ctx.glyph_index = glyph_index;
    ctx.time = time;
    ctx.total_glyphs = static_cast<float>(glyphs.size());
    ctx.total_clusters = static_cast<float>(count_total_clusters(glyphs));
    ctx.total_words = static_cast<float>(count_total_words(glyphs));
    ctx.total_lines = static_cast<float>(lines.size());
    ctx.total_non_space_glyphs = static_cast<float>(count_total_non_space(glyphs, is_space));

    std::size_t non_space_index = 0;
    for (std::size_t i = 0; i < glyph_index && i < glyphs.size(); ++i) {
        if (!is_space(glyphs[i])) {
            ++non_space_index;
        }
    }
    ctx.non_space_glyph_index = static_cast<float>(non_space_index);

    if (glyph_index < glyphs.size()) {
        const auto& glyph = glyphs[glyph_index];
        ctx.cluster_index = glyph.cluster_index;
        ctx.word_index = glyph.word_index;
        ctx.line_index = find_line(lines, glyph_index);
        ctx.is_space = is_space(glyph);
        ctx.is_rtl = glyph.is_rtl;
        populate_extra(ctx, glyph);
    }

    return ctx;
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
    return build_context_common(
        glyph_index,
        time,
        glyphs,
        lines,
        [](const ResolvedGlyph& glyph) { return glyph.is_space || glyph.whitespace; },
        [](const std::vector<ResolvedTextLine>&, std::size_t) { return std::size_t{0}; },
        [](TextAnimatorContext&, const ResolvedGlyph&) {});
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
    return build_context_common(
        glyph_index,
        time,
        glyphs,
        lines,
        [](const PositionedGlyph& glyph) { return glyph.whitespace; },
        find_line_index_tl,
        [](TextAnimatorContext& ctx, const PositionedGlyph& glyph) {
            ctx.cluster_codepoint_start = glyph.cluster_codepoint_start;
            ctx.cluster_codepoint_count = glyph.cluster_codepoint_count;
        });
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
