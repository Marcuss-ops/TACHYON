#include "tachyon/text/animation/text_animator_utils.h"
#include "tachyon/core/animation/easing.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace tachyon::text {

namespace {
constexpr float kTwoPi = 6.28318530717958647692f;
constexpr float kZero = 0.0f;
constexpr float kOne = 1.0f;
constexpr float kHalf = 0.5f;
constexpr float kQuarter = 0.25f;
constexpr float kPercent = 100.0f;
constexpr float kCoverageEpsilon = 1.0e-6f;
constexpr float kMinWavePeriod = 0.001f;
constexpr float kMinGlyphScale = 0.05f;
constexpr float kPhaseOffsetPerGlyph = 0.1f;
constexpr float kRandomSeedScale = 1000.0f;
constexpr std::uint64_t kRandomSeedOffset = 12345ULL;
constexpr std::uint64_t kRandomMixA = 6364136223846793005ULL;
constexpr std::uint64_t kRandomMixB = 1442695040888963407ULL;
constexpr std::uint64_t kRandomSeedDefault = 0xcafebabeULL;
constexpr std::uint64_t kRandomUnitMask = 0xFFFFFFFFULL;
constexpr std::size_t kRandomSeedModA = 100;
constexpr std::size_t kRandomSeedModB = 137;

float parse_float_token(const std::string& token) {
    const char* begin = token.c_str();
    char* end = nullptr;
    const float parsed = std::strtof(begin, &end);
    if (end == begin || *end != '\0') {
        return kZero;
    }
    return parsed;
}

template <typename T>
T lerp_channel(T a, T b, float t) {
    return static_cast<T>(a + (b - a) * t);
}

::tachyon::ColorSpec blend_color(const ::tachyon::ColorSpec& a, const ::tachyon::ColorSpec& b, float t) {
    const float clamped = std::clamp(t, kZero, kOne);
    return {
        lerp_channel<std::uint8_t>(a.r, b.r, clamped),
        lerp_channel<std::uint8_t>(a.g, b.g, clamped),
        lerp_channel<std::uint8_t>(a.b, b.b, clamped),
        lerp_channel<std::uint8_t>(a.a, b.a, clamped)
    };
}

// Expression selector - evaluates a simple expression string
// Supported variables: index, time, total_glyphs, total_clusters, total_lines, word_index, line_index
// Supported operations: +, -, *, /, %, <, >, <=, >=, ==, !=, &&, ||, !, ?: (ternary), sin, cos, abs, floor, ceil, random
// Example: "index / total_glyphs" or "time > 2 ? 1 : 0" or "random(index) * 0.5"
float evaluate_expression(const std::string& expr, const TextAnimatorContext& ctx) {
    if (expr.empty()) return kOne;
    
    // Simple recursive descent parser for expressions
    class ExpressionParser {
    public:
        ExpressionParser(const std::string& e, const TextAnimatorContext& c) : expr(e), ctx(c), pos(0) {}
        
        float parse() {
            float result = parse_ternary();
            skip_whitespace();
            return result;
        }
        
    private:
        std::string expr;
        const TextAnimatorContext& ctx;
        size_t pos;
        
        void skip_whitespace() {
            while (pos < expr.size() && std::isspace(expr[pos])) pos++;
        }
        
        float parse_ternary() {
            float cond = parse_logical_or();
            skip_whitespace();
            if (pos < expr.size() && expr[pos] == '?') {
                pos++; // skip '?'
                float true_val = parse_ternary();
                skip_whitespace();
                if (pos < expr.size() && expr[pos] == ':') {
                    pos++; // skip ':'
                    float false_val = parse_ternary();
                    return cond != 0.0f ? true_val : false_val;
                }
            }
            return cond;
        }
        
        float parse_logical_or() {
            float left = parse_logical_and();
            skip_whitespace();
            while (pos + 1 < expr.size() && expr.substr(pos, 2) == "||") {
                pos += 2;
                float right = parse_logical_and();
                left = (left != 0.0f || right != 0.0f) ? 1.0f : 0.0f;
                skip_whitespace();
            }
            return left;
        }
        
        float parse_logical_and() {
            float left = parse_equality();
            skip_whitespace();
            while (pos + 1 < expr.size() && expr.substr(pos, 2) == "&&") {
                pos += 2;
                float right = parse_equality();
                left = (left != 0.0f && right != 0.0f) ? 1.0f : 0.0f;
                skip_whitespace();
            }
            return left;
        }
        
        float parse_equality() {
            float left = parse_comparison();
            skip_whitespace();
            while (pos + 1 < expr.size()) {
                if (expr.substr(pos, 2) == "==") {
                    pos += 2;
                    float right = parse_comparison();
                    left = (left == right) ? 1.0f : 0.0f;
                } else if (expr.substr(pos, 2) == "!=") {
                    pos += 2;
                    float right = parse_comparison();
                    left = (left != right) ? 1.0f : 0.0f;
                } else break;
                skip_whitespace();
            }
            return left;
        }
        
        float parse_comparison() {
            float left = parse_addition();
            skip_whitespace();
            while (pos + 1 < expr.size()) {
                if (expr.substr(pos, 2) == "<=") {
                    pos += 2;
                    float right = parse_addition();
                    left = (left <= right) ? 1.0f : 0.0f;
                } else if (expr.substr(pos, 2) == ">=") {
                    pos += 2;
                    float right = parse_addition();
                    left = (left >= right) ? 1.0f : 0.0f;
                } else if (expr[pos] == '<') {
                    pos++;
                    float right = parse_addition();
                    left = (left < right) ? 1.0f : 0.0f;
                } else if (expr[pos] == '>') {
                    pos++;
                    float right = parse_addition();
                    left = (left > right) ? 1.0f : 0.0f;
                } else break;
                skip_whitespace();
            }
            return left;
        }
        
        float parse_addition() {
            float left = parse_multiplication();
            skip_whitespace();
            while (pos < expr.size() && (expr[pos] == '+' || expr[pos] == '-')) {
                char op = expr[pos++];
                float right = parse_multiplication();
                left = (op == '+') ? left + right : left - right;
                skip_whitespace();
            }
            return left;
        }
        
        float parse_multiplication() {
            float left = parse_unary();
            skip_whitespace();
            while (pos < expr.size() && (expr[pos] == '*' || expr[pos] == '/' || expr[pos] == '%')) {
                char op = expr[pos++];
                float right = parse_unary();
                if (op == '*') left *= right;
                else if (op == '/') left = (right != 0.0f) ? left / right : 0.0f;
                else left = std::fmod(left, right);
                skip_whitespace();
            }
            return left;
        }
        
        float parse_unary() {
            skip_whitespace();
            if (pos < expr.size() && expr[pos] == '!') {
                pos++;
                float val = parse_unary();
                return (val == 0.0f) ? 1.0f : 0.0f;
            }
            if (pos < expr.size() && expr[pos] == '-') {
                pos++;
                return -parse_unary();
            }
            if (pos < expr.size() && expr[pos] == '+') {
                pos++;
                return parse_unary();
            }
            return parse_primary();
        }
        
        float parse_primary() {
            skip_whitespace();
            if (pos >= expr.size()) return 0.0f;
            
            // Number
            if (std::isdigit(expr[pos]) || expr[pos] == '.') {
                size_t start = pos;
                while (pos < expr.size() && (std::isdigit(expr[pos]) || expr[pos] == '.')) pos++;
                return parse_float_token(expr.substr(start, pos - start));
            }
            
            // Parenthesized expression
            if (expr[pos] == '(') {
                pos++; // skip '('
                float val = parse_ternary();
                skip_whitespace();
                if (pos < expr.size() && expr[pos] == ')') pos++;
                return val;
            }
            
            // Identifier or function call
            if (std::isalpha(expr[pos]) || expr[pos] == '_') {
                size_t start = pos;
                while (pos < expr.size() && (std::isalnum(expr[pos]) || expr[pos] == '_')) pos++;
                std::string id = expr.substr(start, pos - start);
                
                // Check for function call
                skip_whitespace();
                if (pos < expr.size() && expr[pos] == '(') {
                    pos++; // skip '('
                    float arg = parse_ternary();
                    skip_whitespace();
                    if (pos < expr.size() && expr[pos] == ')') pos++;
                    
                    if (id == "sin") return std::sin(arg);
                    if (id == "cos") return std::cos(arg);
                    if (id == "abs") return std::abs(arg);
                    if (id == "floor") return std::floor(arg);
                    if (id == "ceil") return std::ceil(arg);
                    if (id == "random") {
                        std::uint64_t seed = static_cast<std::uint64_t>(arg * kRandomSeedScale) + kRandomSeedOffset;
                        seed = seed * kRandomMixA + kRandomMixB;
                        return static_cast<float>(seed >> 32) / static_cast<float>(kRandomUnitMask);
                    }
                    return 0.0f;
                }
                
                // Variable lookup
                if (id == "index") return static_cast<float>(ctx.glyph_index);
                if (id == "time") return ctx.time;
                if (id == "total_glyphs") return ctx.total_glyphs;
                if (id == "total_clusters") return ctx.total_clusters;
                if (id == "total_lines") return ctx.total_lines;
                if (id == "word_index") return static_cast<float>(ctx.word_index);
                if (id == "line_index") return static_cast<float>(ctx.line_index);
                
                return 0.0f;
            }
            
            return 0.0f;
        }
    };
    
    ExpressionParser parser(expr, ctx);
    return std::clamp(parser.parse(), kZero, kOne);
}

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
        const std::size_t end_idx = selector.end_index.value_or(ctx.total_glyphs);
        std::size_t idx = ctx.glyph_index;
        
        // For cluster-based selection, use cluster_index to preserve shaping
        if (selector.based_on == "clusters") {
            idx = ctx.cluster_index;
        } else if (selector.based_on == "words" && ctx.total_clusters > kZero) {
            idx = ctx.word_index;
        } else if (selector.based_on == "lines" && ctx.total_lines > kZero) {
            idx = ctx.line_index;
        } else if (selector.based_on == "characters_excluding_spaces") {
            // For excluding spaces, we need to compute a non-space character index
            // This is approximated using glyph_index but callers should pre-compute
            idx = ctx.glyph_index;
        }
        return (idx >= start_idx && idx < end_idx) ? kOne : kZero;
    }

    // Random selector
    if (selector.type == "random") {
        std::uint64_t seed = selector.seed.value_or(0) + static_cast<std::uint64_t>(ctx.glyph_index);
        seed = seed * kRandomMixA + kRandomMixB;
        const float rand_val = static_cast<float>(seed >> 32) / static_cast<float>(kRandomUnitMask);
        return rand_val * static_cast<float>(selector.amount.value_or(kPercent) / kPercent);
    }

    // Wiggly selector (time-based noise)
    if (selector.type == "wiggly") {
        const float freq = static_cast<float>(selector.frequency.value_or(2.0));
        const float amp = static_cast<float>(selector.amount.value_or(kPercent) / kPercent);
        std::uint64_t seed = selector.seed.value_or(kRandomSeedDefault) ^ static_cast<std::uint64_t>(ctx.glyph_index);
        double t_noise = static_cast<double>(ctx.time) * freq;
        float n = std::sin(static_cast<float>(t_noise) + static_cast<float>(seed % kRandomSeedModA)) * kHalf + kHalf;
        n += std::sin(static_cast<float>(t_noise * 2.1) + static_cast<float>(seed % kRandomSeedModB)) * kQuarter;
        return std::clamp(n * amp, kZero, kOne);
    }

    // Expression selector
    if (selector.type == "expression" && selector.expression.has_value()) {
        return evaluate_expression(*selector.expression, ctx);
    }

    // Compute normalized position t based on based_on mode
    float t = kZero;
    
    if (selector.based_on == "clusters") {
        // Cluster-based: preserves shaping boundaries
        t = (ctx.total_clusters > kOne) ? static_cast<float>(ctx.cluster_index) / static_cast<float>(ctx.total_clusters - kOne) : kZero;
    } else if (selector.based_on == "words" && ctx.total_clusters > kZero) {
        t = (ctx.total_clusters > kOne) ? static_cast<float>(ctx.word_index) / static_cast<float>(ctx.total_clusters - kOne) : kZero;
    } else if (selector.based_on == "lines" && ctx.total_lines > kZero) {
        t = (ctx.total_lines > kOne) ? static_cast<float>(ctx.line_index) / static_cast<float>(ctx.total_lines - kOne) : kZero;
    } else if (selector.based_on == "characters_excluding_spaces") {
        // For non-space characters, we use the glyph index but this should be
        // a consecutive index that skips spaces (callers need to pre-compute)
        t = (ctx.total_glyphs > kOne) ? static_cast<float>(ctx.glyph_index) / static_cast<float>(ctx.total_glyphs - kOne) : kZero;
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
