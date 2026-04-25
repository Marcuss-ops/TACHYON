#include "tachyon/text/animation/text_animator_utils.h"
#include <cmath>
#include <cstdlib>

namespace tachyon::text {

namespace {

constexpr float kTwoPi = 6.28318530717958647692f;
constexpr float kZero = 0.0f;
constexpr float kOne = 1.0f;
constexpr std::uint64_t kRandomSeedScale = 1000.0f;
constexpr std::uint64_t kRandomSeedOffset = 12345ULL;
constexpr std::uint64_t kRandomMixA = 6364136223846793005ULL;
constexpr std::uint64_t kRandomMixB = 1442695040888963407ULL;
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

// Expression parser for text animator selectors
float evaluate_expression(const std::string& expr, const TextAnimatorContext& ctx) {
    if (expr.empty()) return kOne;
    
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
                pos++;
                float true_val = parse_ternary();
                skip_whitespace();
                if (pos < expr.size() && expr[pos] == ':') {
                    pos++;
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
            
            if (std::isdigit(expr[pos]) || expr[pos] == '.') {
                size_t start = pos;
                while (pos < expr.size() && (std::isdigit(expr[pos]) || expr[pos] == '.')) pos++;
                return parse_float_token(expr.substr(start, pos - start));
            }
            
            if (expr[pos] == '(') {
                pos++;
                float val = parse_ternary();
                skip_whitespace();
                if (pos < expr.size() && expr[pos] == ')') pos++;
                return val;
            }
            
            if (std::isalpha(expr[pos]) || expr[pos] == '_') {
                size_t start = pos;
                while (pos < expr.size() && (std::isalnum(expr[pos]) || expr[pos] == '_')) pos++;
                std::string id = expr.substr(start, pos - start);
                
                skip_whitespace();
                if (pos < expr.size() && expr[pos] == '(') {
                    pos++;
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
                
                if (id == "index") return static_cast<float>(ctx.glyph_index);
                if (id == "time") return ctx.time;
                if (id == "total_glyphs") return ctx.total_glyphs;
                if (id == "total_clusters") return ctx.total_clusters;
                if (id == "total_words") return ctx.total_words;
                if (id == "total_lines") return ctx.total_lines;
                if (id == "non_space_index") return ctx.non_space_glyph_index;
                if (id == "total_non_space_glyphs") return ctx.total_non_space_glyphs;
                if (id == "word_index") return static_cast<float>(ctx.word_index);
                if (id == "line_index") return static_cast<float>(ctx.line_index);
                if (id == "is_rtl") return ctx.is_rtl ? 1.0f : 0.0f;
                if (id == "is_space" || id == "whitespace") return ctx.is_space ? 1.0f : 0.0f;
                
                return 0.0f;
            }
            
            return 0.0f;
        }
    };
    
    ExpressionParser parser(expr, ctx);
    return std::clamp(parser.parse(), kZero, kOne);
}

} // namespace

// Export for use in text_animator_coverage.cpp
float evaluate_expression_wrapper(const std::string& expr, const TextAnimatorContext& ctx) {
    return evaluate_expression(expr, ctx);
}

} // namespace tachyon::text
