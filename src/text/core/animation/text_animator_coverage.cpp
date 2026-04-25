#include "tachyon/text/animation/text_animator_utils.h"
#include <cmath>

namespace tachyon::text {

namespace {

constexpr float kZero = 0.0f;
constexpr float kOne = 1.0f;
constexpr float kHalf = 0.5f;
constexpr float kQuarter = 0.25f;
constexpr float kPercent = 100.0f;
constexpr float kCoverageEpsilon = 1.0e-6f;
constexpr float kMinWavePeriod = 0.001f;
constexpr float kMinGlyphScale = 0.05f;
constexpr float kPhaseOffsetPerGlyph = 0.1f;
constexpr std::uint64_t kRandomSeedScale = 1000ULL;
constexpr std::uint64_t kRandomSeedOffset = 12345ULL;
constexpr std::uint64_t kRandomMixA = 6364136223846793005ULL;
constexpr std::uint64_t kRandomMixB = 1442695040888963407ULL;
constexpr std::uint64_t kRandomUnitMask = 0xFFFFFFFFULL;
constexpr std::size_t kRandomSeedModA = 100;
constexpr std::size_t kRandomSeedModB = 137;

} // namespace

float compute_coverage(const TextAnimatorSelectorSpec& selector, const TextAnimatorContext& ctx) {
    if (selector.type == "all" || ctx.total_glyphs <= kZero) return kOne;

    // For characters_excluding_spaces, skip spaces entirely (return 0 coverage)
    if (selector.based_on == "characters_excluding_spaces" && ctx.is_space) {
        return kZero;
    }

    // Index selector: explicit start/end index
    if (selector.type == "index") {
        const std::size_t start_idx = selector.start_index.value_or(0);
        const std::size_t fallback_end =
            selector.based_on == "clusters" ? static_cast<std::size_t>(ctx.total_clusters) :
            selector.based_on == "words" ? static_cast<std::size_t>(ctx.total_words) :
            selector.based_on == "lines" ? static_cast<std::size_t>(ctx.total_lines) :
            selector.based_on == "characters_excluding_spaces" ? static_cast<std::size_t>(ctx.total_non_space_glyphs) :
            static_cast<std::size_t>(ctx.total_glyphs);
        const std::size_t end_idx = selector.end_index.value_or(fallback_end);
        std::size_t idx = ctx.glyph_index;

        // For cluster-based selection, use cluster_index to preserve shaping
        if (selector.based_on == "clusters") {
            idx = ctx.cluster_index;
        } else if (selector.based_on == "words" && ctx.total_words > kZero) {
            idx = ctx.word_index;
        } else if (selector.based_on == "lines" && ctx.total_lines > kZero) {
            idx = ctx.line_index;
        } else if (selector.based_on == "characters_excluding_spaces") {
            idx = ctx.is_space
                ? static_cast<std::size_t>(ctx.total_non_space_glyphs)
                : static_cast<std::size_t>(ctx.non_space_glyph_index);
        }
        return (idx >= start_idx && idx < end_idx) ? kOne : kZero;
    }

    // Random selector
    if (selector.type == "random") {
        std::uint64_t seed = selector.seed.value_or(0) + static_cast<std::uint64_t>(ctx.glyph_index);
        seed = seed * kRandomMixA + kRandomMixB;
        const float rand_val = static_cast<float>(seed >> 32) / static_cast<float>(kRandomUnitMask);
        return rand_val * static_cast<float>(selector.amount.value_or(kPercent)) / kPercent;
    }

    // Wiggly selector (time-based noise)
    if (selector.type == "wiggly") {
        const float freq = static_cast<float>(selector.frequency.value_or(2.0));
        const float amp = static_cast<float>(selector.amount.value_or(kPercent)) / kPercent;
        std::uint64_t seed = selector.seed.value_or(0xCAFEBABEULL) ^ static_cast<std::uint64_t>(ctx.glyph_index);
        double t_noise = static_cast<double>(ctx.time) * freq;
        float n = std::sin(static_cast<float>(t_noise) + static_cast<float>(seed % kRandomSeedModA)) * kHalf + kHalf;
        n += std::sin(static_cast<float>(t_noise * 2.1) + static_cast<float>(seed % kRandomSeedModB)) * kQuarter;
        return std::clamp(n * amp, kZero, kOne);
    }

    // Expression selector
    if (selector.type == "expression" && selector.expression.has_value()) {
        // Use the expression evaluator from text_animator_expression.cpp
        return kOne; // Placeholder - actual implementation calls evaluate_expression
    }

    // Compute normalized position t based on based_on mode
    float t = kZero;

    if (selector.based_on == "clusters") {
        t = (ctx.total_clusters > kOne) ? static_cast<float>(ctx.cluster_index) / static_cast<float>(ctx.total_clusters - kOne) : kZero;
    } else if (selector.based_on == "words" && ctx.total_words > kZero) {
        t = (ctx.total_words > kOne) ? static_cast<float>(ctx.word_index) / static_cast<float>(ctx.total_words - kOne) : kZero;
    } else if (selector.based_on == "lines" && ctx.total_lines > kZero) {
        t = (ctx.total_lines > kOne) ? static_cast<float>(ctx.line_index) / static_cast<float>(ctx.total_lines - kOne) : kZero;
    } else if (selector.based_on == "characters_excluding_spaces") {
        if (ctx.total_non_space_glyphs > kZero && !ctx.is_space) {
            t = (ctx.total_non_space_glyphs > kOne)
                ? static_cast<float>(ctx.non_space_glyph_index) / static_cast<float>(ctx.total_non_space_glyphs - kOne)
                : kZero;
        } else {
            t = kZero;
        }
    } else {
        t = (ctx.total_glyphs > kOne) ? static_cast<float>(ctx.glyph_index) / static_cast<float>(ctx.total_glyphs - kOne) : kZero;
    }

    const float start = static_cast<float>(selector.start) / kPercent;
    const float end   = static_cast<float>(selector.end) / kPercent;
    const float span  = end - start;

    if (std::abs(span) < kCoverageEpsilon) return (t >= start) ? kOne : kZero;

    float coverage = std::clamp((t - start) / span, kZero, kOne);

    if (selector.mode == "subtract") return kOne - coverage;
    return coverage;
}

} // namespace tachyon::text
