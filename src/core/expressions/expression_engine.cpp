#include "tachyon/core/expressions/expression_engine.h"
#include "tachyon/runtime/core/contracts/math_contract.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cctype>
#include <stdexcept>

namespace tachyon {
namespace expressions {

namespace {

/**
 * @brief Deterministic 1D Smooth Noise (Cubic interpolation between hashed points).
 */
double smooth_noise_1d(double x, std::uint64_t seed) {
    const std::uint64_t i = static_cast<std::uint64_t>(std::floor(x));
    const double f = x - static_cast<double>(i);
    const double u = f * f * (3.0 - 2.0 * f); // Cubic easing

    const double v0 = math_contract::noise(i + seed);
    const double v1 = math_contract::noise(i + 1 + seed);
    
    return math_contract::lerp(v0, v1, u) * 2.0 - 1.0; // Map [0,1] to [-1,1]
}

} // namespace

class Parser {
public:
    Parser(const std::string& input, const ExpressionContext& context)
        : m_input(input), m_context(context), m_pos(0) {}

    EvaluationResult parse() {
        try {
            double value = parse_expression();
            skip_whitespace();
            if (m_pos < m_input.length()) {
                throw std::runtime_error("Unexpected characters at end of expression");
            }
            return {value, true, ""};
        } catch (const std::exception& e) {
            return {0.0, false, e.what()};
        }
    }

private:
    std::string m_input;
    const ExpressionContext& m_context;
    std::size_t m_pos;

    char peek() const { return m_pos < m_input.length() ? m_input[m_pos] : '\0'; }
    char get() { return m_pos < m_input.length() ? m_input[m_pos++] : '\0'; }

    void skip_whitespace() {
        while (std::isspace(peek())) m_pos++;
    }

    double parse_expression() {
        double result = parse_term();
        while (true) {
            skip_whitespace();
            if (peek() == '+') { get(); result += parse_term(); }
            else if (peek() == '-') { get(); result -= parse_term(); }
            else break;
        }
        return result;
    }

    double parse_term() {
        double result = parse_factor();
        while (true) {
            skip_whitespace();
            if (peek() == '*') { get(); result *= parse_factor(); }
            else if (peek() == '/') {
                get();
                double divisor = parse_factor();
                if (divisor == 0.0) throw std::runtime_error("Division by zero");
                result /= divisor;
            }
            else break;
        }
        return result;
    }

    double parse_factor() {
        double result = parse_base();
        skip_whitespace();
        if (peek() == '^') {
            get();
            result = std::pow(result, parse_factor());
        }
        return result;
    }

    double parse_base() {
        skip_whitespace();
        char c = peek();

        if (std::isdigit(c) || c == '.') {
            return parse_number();
        } else if (std::isalpha(c)) {
            return parse_identifier_or_function();
        } else if (c == '(') {
            get();
            double result = parse_expression();
            skip_whitespace();
            if (get() != ')') throw std::runtime_error("Missing closing parenthesis");
            return result;
        } else if (c == '-') {
            get();
            return -parse_base();
        } else if (c == '+') {
            get();
            return parse_base();
        }

        throw std::runtime_error("Expected number, variable, or parenthesis");
    }

    double parse_number() {
        size_t start = m_pos;
        while (std::isdigit(peek()) || peek() == '.') get();
        std::string s = m_input.substr(start, m_pos - start);
        
        // Use a pointer to satisfy locale-independent parsing if needed, 
        // Replace commas with dots before std::stod parsing.
        std::replace(s.begin(), s.end(), ',', '.');
        return std::stod(s); 
    }

    double parse_identifier_or_function() {
        std::size_t start = m_pos;
        while (std::isalnum(peek()) || peek() == '_' || peek() == '.') get();
        std::string name = m_input.substr(start, m_pos - start);

        skip_whitespace();
        if (peek() == '(') {
            get(); // consume '('
            std::vector<double> args;
            if (peek() != ')') {
                while (true) {
                    args.push_back(parse_expression());
                    skip_whitespace();
                    if (peek() == ',') get();
                    else break;
                }
            }
            if (get() != ')') throw std::runtime_error("Missing closing parenthesis for function " + name);
            return call_function(name, args);
        }

        auto it = m_context.variables.find(name);
        if (it != m_context.variables.end()) {
            return it->second;
        }

        throw std::runtime_error("Unknown variable: " + name);
    }

    double call_function(const std::string& name, const std::vector<double>& args) {
        if (name == "sin") return (args.size() == 1) ? std::sin(args[0]) : throw std::runtime_error("sin() expects 1 argument");
        if (name == "cos") return (args.size() == 1) ? std::cos(args[0]) : throw std::runtime_error("cos() expects 1 argument");
        if (name == "abs") return (args.size() == 1) ? std::abs(args[0]) : throw std::runtime_error("abs() expects 1 argument");
        if (name == "tan") return (args.size() == 1) ? std::tan(args[0]) : throw std::runtime_error("tan() expects 1 argument");
        if (name == "min") return (args.size() == 2) ? std::min(args[0], args[1]) : throw std::runtime_error("min() expects 2 arguments");
        if (name == "max") return (args.size() == 2) ? std::max(args[0], args[1]) : throw std::runtime_error("max() expects 2 arguments");
        if (name == "clamp") return (args.size() == 3) ? std::clamp(args[0], args[1], args[2]) : throw std::runtime_error("clamp() expects 3 arguments");
        
        if (name == "lerp" || name == "interpolate") {
             if (args.size() != 3) throw std::runtime_error(name + "() expects 3 arguments (a, b, t)");
             return args[0] + (args[1] - args[0]) * args[2];
        }

        if (name == "spring") {
            if (args.size() != 5) throw std::runtime_error("spring() expects 5 arguments (t, from, to, freq, damping)");
            return math_contract::spring(args[0], args[1], args[2], args[3], args[4]);
        }

        if (name == "pingpong") {
            if (args.size() != 2) throw std::runtime_error("pingpong() expects 2 arguments (time, duration)");
            const double t = args[0];
            const double duration = args[1];
            if (duration <= 0.0) return t;
            const double fold = std::floor(t / duration);
            const double rem = std::fmod(t, duration);
            return (static_cast<std::uint64_t>(fold) % 2 == 0) ? rem : (duration - rem);
        }

        if (name == "timeStretch") {
            if (args.size() != 2) throw std::runtime_error("timeStretch() expects 2 arguments (time, scale)");
            return args[0] * args[1];
        }

        if (name == "stagger") {
            if (args.size() != 1) throw std::runtime_error("stagger() expects 1 argument (delay)");
            return static_cast<double>(m_context.layer_index) * args[0];
        }

        if (name == "valueAtTime") {
            if (args.size() != 2) throw std::runtime_error("valueAtTime() expects 2 arguments (property_index, time)");
            if (m_context.property_sampler) {
                return m_context.property_sampler(static_cast<int>(args[0]), args[1]);
            }
            return 0.0; 
        }
        if (name == "wiggle") {
            if (args.size() < 2 || args.size() > 5) throw std::runtime_error("wiggle() expects 2-5 arguments (freq, amp, [octaves], [amp_mult], [t])");
            const double freq = args[0];
            const double amp = args[1];
            const int octaves = (args.size() >= 3) ? static_cast<int>(args[2]) : 1;
            const double amp_mult = (args.size() >= 4) ? args[3] : 0.5;
            const double t = (args.size() >= 5) ? args[4] : m_context.variables.at("t");

            double value = 0.0;
            double current_amp = 1.0;
            double current_freq = freq;
            double total_amp = 0.0;

            for (int i = 0; i < std::max(1, octaves); ++i) {
                value += smooth_noise_1d(t * current_freq, m_context.seed + i) * current_amp;
                total_amp += current_amp;
                current_freq *= 2.0;
                current_amp *= amp_mult;
            }

            return (value / (total_amp > 0 ? total_amp : 1.0)) * amp;
        }

        if (name == "loopOut") {
            const double t = m_context.variables.at("t");
            if (m_context.variables.count("_prop_duration") && m_context.property_sampler) {
                const double start = m_context.variables.at("_prop_start");
                const double end = m_context.variables.at("_prop_end");
                const double duration = m_context.variables.at("_prop_duration");
                
                if (duration <= 0.0) return m_context.property_sampler(-1, t);
                if (t <= end) return m_context.property_sampler(-1, t);

                const double phase = std::fmod(t - end, duration);
                return m_context.property_sampler(-1, start + phase);
            }
            return 0.0;
        }

        if (name == "linear" || name == "ease") {
            if (args.size() != 5) throw std::runtime_error(name + "() expects 5 arguments (t, tMin, tMax, value1, value2)");
            const double t = args[0];
            const double tMin = args[1];
            const double tMax = args[2];
            const double v1 = args[3];
            const double v2 = args[4];

            if (tMin >= tMax) return (t <= tMin) ? v1 : v2;
            
            double alpha = std::clamp((t - tMin) / (tMax - tMin), 0.0, 1.0);
            if (name == "ease") {
                alpha = alpha * alpha * (3.0 - 2.0 * alpha); // Cubic easing
            }
            
            return v1 + alpha * (v2 - v1);
        }

        if (name == "data") {
            if (args.size() != 3) throw std::runtime_error("data() expects 3 arguments (table_idx, row, col)");
            if (m_context.table_lookup) {
                return m_context.table_lookup(args[0], args[1], args[2]);
            }
            return 0.0;
        }
        
        throw std::runtime_error("Unknown function: " + name);
    }
};

EvaluationResult ExpressionEvaluator::evaluate(const std::string& expression, const ExpressionContext& context) {
    if (expression.empty()) return {0.0, false, "Empty expression"};
    ExpressionContext resolved = context;
    
    // Inject global context variables if not already present
    if (resolved.variables.find("seed") == resolved.variables.end()) {
        resolved.variables["seed"] = static_cast<double>(resolved.seed);
    }
    if (resolved.variables.find("index") == resolved.variables.end()) {
        resolved.variables["index"] = static_cast<double>(resolved.layer_index);
    }
    if (resolved.variables.find("value") == resolved.variables.end()) {
        resolved.variables["value"] = resolved.value;
    }

    Parser parser(expression, resolved);
    return parser.parse();
}

} // namespace expressions
} // namespace tachyon
