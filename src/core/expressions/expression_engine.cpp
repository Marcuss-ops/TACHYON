#include "tachyon/core/expressions/expression_engine.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cctype>
#include <stdexcept>

namespace tachyon {
namespace expressions {

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
        // but for now, we'll try to replace any ',' with '.' just in case std::stod is finicky.
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
        
        throw std::runtime_error("Unknown function: " + name);
    }
};

EvaluationResult ExpressionEvaluator::evaluate(const std::string& expression, const ExpressionContext& context) {
    if (expression.empty()) return {0.0, false, "Empty expression"};
    ExpressionContext resolved = context;
    if (resolved.variables.find("seed") == resolved.variables.end()) {
        resolved.variables["seed"] = static_cast<double>(resolved.seed);
    }
    Parser parser(expression, resolved);
    return parser.parse();
}

} // namespace expressions
} // namespace tachyon
