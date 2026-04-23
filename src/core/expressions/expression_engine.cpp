#include "tachyon/core/expressions/expression_engine.h"
#include "tachyon/runtime/core/contracts/math_contract.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cctype>
#include <stdexcept>
#include <unordered_map>

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
    Parser(const std::string& input)
        : m_input(input), m_pos(0) {}

    std::unique_ptr<ASTNode> parse() {
        auto ast = parse_expression();
        skip_whitespace();
        if (m_pos < m_input.length()) {
            throw std::runtime_error("Unexpected characters at end of expression");
        }
        return ast;
    }

private:
    std::string m_input;
    std::size_t m_pos;

    char peek() const { return m_pos < m_input.length() ? m_input[m_pos] : '\0'; }
    char get() { return m_pos < m_input.length() ? m_input[m_pos++] : '\0'; }

    void skip_whitespace() {
        while (std::isspace(peek())) m_pos++;
    }

    std::unique_ptr<ASTNode> parse_expression() {
        auto result = parse_term();
        while (true) {
            skip_whitespace();
            if (peek() == '+') { 
                get(); 
                result = std::make_unique<BinaryOpNode>(BinaryOperator::Add, std::move(result), parse_term()); 
            }
            else if (peek() == '-') { 
                get(); 
                result = std::make_unique<BinaryOpNode>(BinaryOperator::Sub, std::move(result), parse_term()); 
            }
            else break;
        }
        return result;
    }

    std::unique_ptr<ASTNode> parse_term() {
        auto result = parse_factor();
        while (true) {
            skip_whitespace();
            if (peek() == '*') { 
                get(); 
                result = std::make_unique<BinaryOpNode>(BinaryOperator::Mul, std::move(result), parse_factor()); 
            }
            else if (peek() == '/') {
                get();
                result = std::make_unique<BinaryOpNode>(BinaryOperator::Div, std::move(result), parse_factor()); 
            }
            else break;
        }
        return result;
    }

    std::unique_ptr<ASTNode> parse_factor() {
        auto result = parse_base();
        skip_whitespace();
        if (peek() == '^') {
            get();
            result = std::make_unique<BinaryOpNode>(BinaryOperator::Pow, std::move(result), parse_factor());
        }
        return result;
    }

    std::unique_ptr<ASTNode> parse_base() {
        skip_whitespace();
        char c = peek();

        if (std::isdigit(c) || c == '.') {
            return parse_number();
        } else if (std::isalpha(c) || c == '_') {
            return parse_identifier_or_function();
        } else if (c == '(') {
            get();
            auto result = parse_expression();
            skip_whitespace();
            if (get() != ')') throw std::runtime_error("Missing closing parenthesis");
            return result;
        } else if (c == '-') {
            get();
            return std::make_unique<UnaryOpNode>(UnaryOperator::Negate, parse_base());
        } else if (c == '+') {
            get();
            return parse_base();
        }

        throw std::runtime_error(std::string("Expected number, variable, or parenthesis at: ") + peek());
    }

    std::unique_ptr<ASTNode> parse_number() {
        size_t start = m_pos;
        while (std::isdigit(peek()) || peek() == '.') get();
        std::string s = m_input.substr(start, m_pos - start);
        
        std::replace(s.begin(), s.end(), ',', '.');
        return std::make_unique<NumberNode>(std::stod(s)); 
    }

    std::unique_ptr<ASTNode> parse_identifier_or_function() {
        std::size_t start = m_pos;
        while (std::isalnum(peek()) || peek() == '_' || peek() == '.') get();
        std::string name = m_input.substr(start, m_pos - start);

        skip_whitespace();
        if (peek() == '(') {
            get(); // consume '('
            std::vector<std::unique_ptr<ASTNode>> args;
            if (peek() != ')') {
                while (true) {
                    args.push_back(parse_expression());
                    skip_whitespace();
                    if (peek() == ',') get();
                    else break;
                }
            }
            if (get() != ')') throw std::runtime_error("Missing closing parenthesis for function " + name);
            return std::make_unique<FunctionCallNode>(name, std::move(args));
        }
        
        // Check if it's a property access (e.g. thisComp.layer...)
        // For simplicity, we just treat the whole "thisComp.layer.transform.position" as a single VariableNode here
        // A true AST would split it into PropertyAccessNode. Let's stick to VariableNode for the entire path.
        return std::make_unique<VariableNode>(name);
    }
};

class Compiler {
public:
    Compiler(Bytecode& bytecode) : m_bytecode(bytecode) {}

    void compile(ASTNode* node) {
        if (!node) return;
        
        switch (node->type()) {
            case ASTNodeType::Number: {
                auto* n = static_cast<NumberNode*>(node);
                std::uint32_t idx = add_constant(n->value);
                m_bytecode.instructions.push_back({OpCode::PushConst, idx});
                break;
            }
            case ASTNodeType::Variable: {
                auto* v = static_cast<VariableNode*>(node);
                std::uint32_t idx = add_name(v->name);
                m_bytecode.instructions.push_back({OpCode::PushVar, idx});
                break;
            }
            case ASTNodeType::BinaryOp: {
                auto* b = static_cast<BinaryOpNode*>(node);
                compile(b->left.get());
                compile(b->right.get());
                switch (b->op) {
                    case BinaryOperator::Add: m_bytecode.instructions.push_back({OpCode::Add, 0}); break;
                    case BinaryOperator::Sub: m_bytecode.instructions.push_back({OpCode::Sub, 0}); break;
                    case BinaryOperator::Mul: m_bytecode.instructions.push_back({OpCode::Mul, 0}); break;
                    case BinaryOperator::Div: m_bytecode.instructions.push_back({OpCode::Div, 0}); break;
                    case BinaryOperator::Pow: m_bytecode.instructions.push_back({OpCode::Pow, 0}); break;
                }
                break;
            }
            case ASTNodeType::UnaryOp: {
                auto* u = static_cast<UnaryOpNode*>(node);
                compile(u->operand.get());
                if (u->op == UnaryOperator::Negate) {
                    m_bytecode.instructions.push_back({OpCode::Neg, 0});
                }
                break;
            }
            case ASTNodeType::FunctionCall: {
                auto* f = static_cast<FunctionCallNode*>(node);
                // Push arguments (left to right, but VM pops right to left, so this is correct)
                for (auto& arg : f->arguments) {
                    compile(arg.get());
                }
                std::uint32_t name_idx = add_name(f->name);
                std::uint32_t arg_count = static_cast<std::uint32_t>(f->arguments.size());
                std::uint32_t instr_data = (arg_count << 16) | (name_idx & 0xFFFF);
                m_bytecode.instructions.push_back({OpCode::Call, instr_data});
                break;
            }
            case ASTNodeType::PropertyAccess: {
                // Not implemented in this basic compiler pass, fall back to variable string index
                break;
            }
        }
    }

private:
    Bytecode& m_bytecode;

    std::uint32_t add_constant(double val) {
        for (std::size_t i = 0; i < m_bytecode.constants.size(); ++i) {
            if (m_bytecode.constants[i] == val) return static_cast<std::uint32_t>(i);
        }
        m_bytecode.constants.push_back(val);
        return static_cast<std::uint32_t>(m_bytecode.constants.size() - 1);
    }

    std::uint32_t add_name(const std::string& name) {
        for (std::size_t i = 0; i < m_bytecode.names.size(); ++i) {
            if (m_bytecode.names[i] == name) return static_cast<std::uint32_t>(i);
        }
        m_bytecode.names.push_back(name);
        return static_cast<std::uint32_t>(m_bytecode.names.size() - 1);
    }
};

CompilationResult CoreExpressionEvaluator::compile(const std::string& expression) {
    if (expression.empty()) return {{}, nullptr, false, "Empty expression"};
    
    CompilationResult result;
    try {
        Parser parser(expression);
        result.ast = parser.parse();
        
        Compiler compiler(result.bytecode);
        compiler.compile(result.ast.get());
        result.bytecode.instructions.push_back({OpCode::Ret, 0});
        result.success = true;
    } catch (const std::exception& e) {
        result.success = false;
        result.error = e.what();
    }
    return result;
}

EvaluationResult CoreExpressionEvaluator::evaluate(const std::string& expression, const ExpressionContext& context) {
    auto comp = compile(expression);
    if (!comp.success) return {0.0, false, comp.error};
    
    ExpressionContext resolved = context;
    if (resolved.variables.find("seed") == resolved.variables.end()) {
        resolved.variables["seed"] = static_cast<double>(resolved.seed);
    }
    if (resolved.variables.find("index") == resolved.variables.end()) {
        resolved.variables["index"] = static_cast<double>(resolved.layer_index);
    }
    if (resolved.variables.find("value") == resolved.variables.end()) {
        resolved.variables["value"] = resolved.value;
    }

    try {
        double val = ExpressionVM::execute(comp.bytecode, resolved);
        return {val, true, ""};
    } catch (const std::exception& e) {
        return {0.0, false, e.what()};
    }
}

} // namespace expressions
} // namespace tachyon
