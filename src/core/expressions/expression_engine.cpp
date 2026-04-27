#include "tachyon/core/expressions/expression_engine.h"
#include "tachyon/core/animation/animation_primitives.h"
#include "tachyon/runtime/core/contracts/math_contract.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cctype>
#include <stdexcept>
#include <unordered_map>

namespace tachyon {
namespace expressions {

void bind_standard_expression_variables(ExpressionContext& context) {
    context.variables.try_emplace("t", context.time);
    context.variables.try_emplace("time", context.time);
    context.variables.try_emplace("value", context.value);
    context.variables.try_emplace("seed", static_cast<double>(context.seed));
    context.variables.try_emplace("index", static_cast<double>(context.layer_index));
    context.variables.try_emplace("thisProperty.index", static_cast<double>(context.property_index));
    context.variables.try_emplace("thisComp.width", static_cast<double>(context.composition_width));
    context.variables.try_emplace("thisComp.height", static_cast<double>(context.composition_height));
    context.variables.try_emplace("thisComp.time", context.composition_time != 0.0 ? context.composition_time : context.time);
    context.variables.try_emplace("thisLayer.index", static_cast<double>(context.layer_index));
    context.variables.try_emplace("thisLayer.time", context.time);
}

class Parser {
public:
    Parser(const std::string& input)
        : m_input(input), m_pos(0) {}

    std::unique_ptr<ASTNode> parse() {
        auto ast = parse_ternary();
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

    // Ternary: cond ? then : else
    std::unique_ptr<ASTNode> parse_ternary() {
        auto cond = parse_comparison();
        skip_whitespace();
        if (peek() == '?') {
            get();
            auto then_expr = parse_ternary();
            skip_whitespace();
            if (peek() == ':') get();
            auto else_expr = parse_ternary();
            std::vector<std::unique_ptr<ASTNode>> args;
            args.push_back(std::move(cond));
            args.push_back(std::move(then_expr));
            args.push_back(std::move(else_expr));
            return std::make_unique<FunctionCallNode>("__ternary__", std::move(args));
        }
        return cond;
    }

    // Comparison: expr (<|>|<=|>=|==|!=) expr
    std::unique_ptr<ASTNode> parse_comparison() {
        auto result = parse_expression();
        while (true) {
            skip_whitespace();
            char c = peek();
            if (c != '<' && c != '>' && c != '=' && c != '!') break;
            char c2 = (m_pos + 1 < m_input.size()) ? m_input[m_pos + 1] : '\0';
            BinaryOperator op;
            bool matched = true;
            if      (c == '<' && c2 == '=') { op = BinaryOperator::Le; m_pos += 2; }
            else if (c == '>' && c2 == '=') { op = BinaryOperator::Ge; m_pos += 2; }
            else if (c == '=' && c2 == '=') { op = BinaryOperator::Eq; m_pos += 2; }
            else if (c == '!' && c2 == '=') { op = BinaryOperator::Ne; m_pos += 2; }
            else if (c == '<')              { op = BinaryOperator::Lt; m_pos += 1; }
            else if (c == '>')              { op = BinaryOperator::Gt; m_pos += 1; }
            else { matched = false; }
            if (!matched) break;
            result = std::make_unique<BinaryOpNode>(op, std::move(result), parse_expression());
        }
        return result;
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
            else if (peek() == '%') {
                get();
                result = std::make_unique<BinaryOpNode>(BinaryOperator::Mod, std::move(result), parse_factor());
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
        } else if (c == '"' || c == '\'') {
            // String literal
            char quote = get(); // consume opening quote
            std::string value;
            while (peek() != quote && peek() != '\0') {
                if (peek() == '\\') {
                    get(); // consume backslash
                    char esc = get();
                    if (esc == 'n') value += '\n';
                    else if (esc == 't') value += '\t';
                    else if (esc == '\\') value += '\\';
                    else if (esc == quote) value += quote;
                    else value += esc;
                } else {
                    value += get();
                }
            }
            if (get() != quote) throw std::runtime_error("Missing closing quote");
            return std::make_unique<StringNode>(std::move(value));
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
            case ASTNodeType::String: {
                auto* s = static_cast<StringNode*>(node);
                std::uint32_t idx = add_name(s->value);
                m_bytecode.instructions.push_back({OpCode::PushName, idx});
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
                    case BinaryOperator::Mod: m_bytecode.instructions.push_back({OpCode::Mod, 0}); break;
                    case BinaryOperator::Lt:  m_bytecode.instructions.push_back({OpCode::Lt,  0}); break;
                    case BinaryOperator::Gt:  m_bytecode.instructions.push_back({OpCode::Gt,  0}); break;
                    case BinaryOperator::Le:  m_bytecode.instructions.push_back({OpCode::Le,  0}); break;
                    case BinaryOperator::Ge:  m_bytecode.instructions.push_back({OpCode::Ge,  0}); break;
                    case BinaryOperator::Eq:  m_bytecode.instructions.push_back({OpCode::Eq,  0}); break;
                    case BinaryOperator::Ne:  m_bytecode.instructions.push_back({OpCode::Ne,  0}); break;
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
                // Special case: ternary (cond ? then : else)
                if (f->name == "__ternary__" && f->arguments.size() == 3) {
                    compile(f->arguments[0].get());
                    std::size_t jif_pos = m_bytecode.instructions.size();
                    m_bytecode.instructions.push_back({OpCode::JumpIfFalse, 0});
                    compile(f->arguments[1].get());
                    std::size_t jmp_pos = m_bytecode.instructions.size();
                    m_bytecode.instructions.push_back({OpCode::Jump, 0});
                    std::size_t else_start = m_bytecode.instructions.size();
                    m_bytecode.instructions[jif_pos].data = static_cast<std::uint32_t>(else_start);
                    compile(f->arguments[2].get());
                    std::size_t after_else = m_bytecode.instructions.size();
                    m_bytecode.instructions[jmp_pos].data = static_cast<std::uint32_t>(after_else);
                    break;
                }
                // Normal function call
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
    bind_standard_expression_variables(resolved);

    // Populate audio analysis variables for expression access
    resolved.variables["audio.bass"] = resolved.audio_analysis.bass;
    resolved.variables["audio.mid"] = resolved.audio_analysis.mid;
    resolved.variables["audio.treble"] = resolved.audio_analysis.treble;
    resolved.variables["audio.rms"] = resolved.audio_analysis.rms;
    resolved.variables["audio.beat"] = resolved.audio_analysis.beat;

    try {
        double val = ExpressionVM::execute(comp.bytecode, resolved);
        return {val, true, ""};
    } catch (const std::exception& e) {
        return {0.0, false, e.what()};
    }
}

} // namespace expressions
} // namespace tachyon
