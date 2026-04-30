#pragma once
#include <vector>
#include <string>
#include <cstdint>

namespace tachyon::expressions {

enum class OpCode : std::uint8_t {
    PushConst,
    PushVar,
    Add,
    Sub,
    Mul,
    Div,
    Pow,
    Neg,
    Call, // arg = string index for name, high 16 bits = arg count
    Ret
};

struct Instruction {
    OpCode op;
    std::uint32_t data{0};
};

struct Bytecode {
    std::vector<Instruction> instructions;
    std::vector<double> constants;
    std::vector<std::string> names;
};

struct ExpressionContext;

/**
 * @brief Stack-based Virtual Machine for evaluating After Effects expressions.
 * 
 * Note: This is currently an EXPERIMENTAL prototype. It supports basic arithmetic,
 * variables, and a small set of math functions (sin, cos, wiggle, etc.). It does
 * NOT support the full JavaScript specification, objects, arrays, or complex
 * property linking across layers. Performance benchmarks against the AST parser
 * are pending.
 */
class ExpressionVM {
public:
    static double execute(const Bytecode& code, const ExpressionContext& context);
};

} // namespace tachyon::expressions
