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
        
        // Use the newly added noise() logic via the expression engine or directly
        // For efficiency in the hot loop, we use a similar seed-based noise here
        const double t_noise = static_cast<double>(ctx.time) * freq;
        const double glyph_offset = static_cast<double>(ctx.glyph_index) * kPhaseOffsetPerGlyph;
        
        // Composite noise for a more organic "wiggle"
        auto hash = [](int n) {
            n = (n << 13) ^ n;
            return (1.0 - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
        };
        
        double n1 = std::sin(t_noise + glyph_offset);
        double n2 = std::sin(t_noise * 2.1 + glyph_offset * 1.3);
        float n = static_cast<float>((n1 + n2 * 0.5) / 1.5 * kHalf + kHalf);
        
        return std::clamp(n * amp, kZero, kOne);
    }

    // Expression selector
    if (selector.type == "expression" && selector.expression.has_value()) {
        return evaluate_expression_wrapper(selector.expression.value(), ctx);
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

    const float start_p = static_cast<float>(selector.start) / kPercent;
    const float end_p   = static_cast<float>(selector.end) / kPercent;
    const float offset_p = static_cast<float>(selector.offset) / kPercent;

    // Adjusted t by offset (wrap around 0-1)
    float t_adj = t - offset_p;
    while (t_adj < 0.0f) t_adj += 1.0f;
    while (t_adj > 1.0f) t_adj -= 1.0f;

    const float span = end_p - start_p;
    float coverage = 0.0f;

    if (std::abs(span) < kCoverageEpsilon) {
        coverage = (t_adj >= start_p) ? kOne : kZero;
    } else {
        float ramp = (t_adj - start_p) / span;
        
        if (selector.shape == "square") {
            coverage = (ramp >= 0.0f && ramp <= 1.0f) ? kOne : kZero;
        } else {
            ramp = std::clamp(ramp, 0.0f, 1.0f);
            
            if (selector.shape == "ramp_up") {
                coverage = ramp;
            } else if (selector.shape == "ramp_down") {
                coverage = 1.0f - ramp;
            } else if (selector.shape == "triangle") {
                coverage = (ramp <= 0.5f) ? (ramp * 2.0f) : (2.0f - ramp * 2.0f);
            } else if (selector.shape == "round") {
                coverage = std::sqrt(1.0f - std::pow(ramp * 2.0f - 1.0f, 2.0f));
            } else if (selector.shape == "smooth") {
                coverage = ramp * ramp * (3.0f - 2.0f * ramp);
            } else {
                coverage = ramp; // Default to linear ramp_up if unknown
            }
        }
    }

    // Apply AE-style easing (High/Low)
    if (selector.ease_high != 0.0 || selector.ease_low != 0.0) {
        float eh = std::clamp(static_cast<float>(selector.ease_high) / 100.0f, -1.0f, 1.0f);
        float el = std::clamp(static_cast<float>(selector.ease_low) / 100.0f, -1.0f, 1.0f);
        
        // Simple power-based easing approximation for AE's Ease High/Low
        if (coverage > 0.0f && coverage < 1.0f) {
            if (eh > 0.0f) coverage = std::pow(coverage, 1.0f + eh * 2.0f);
            else if (eh < 0.0f) coverage = 1.0f - std::pow(1.0f - coverage, 1.0f - eh * 2.0f);
            
            if (el > 0.0f) coverage = 1.0f - std::pow(1.0f - coverage, 1.0f + el * 2.0f);
            else if (el < 0.0f) coverage = std::pow(coverage, 1.0f - el * 2.0f);
        }
    }

    if (selector.mode == "subtract") return kOne - coverage;
    return coverage;
}

} // namespace tachyon::text
