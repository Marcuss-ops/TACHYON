#include "tachyon/core/expressions/expression_vm.h"
#include "tachyon/core/expressions/expression_engine.h"
#include <iostream>
#include <cmath>
#include <vector>

namespace {

int g_failures = 0;

void check_near(double actual, double expected, const std::string& message, double epsilon = 0.001) {
    if (std::abs(actual - expected) > epsilon) {
        ++g_failures;
        std::cerr << "FAIL: " << message << " (Expected " << expected << ", got " << actual << ")\n";
    }
}

} // namespace

bool run_expression_vm_tests() {
    using namespace tachyon::expressions;

    // 1. Basic Arithmetic: 10 + 20
    {
        Bytecode code;
        code.instructions = {
            {OpCode::PushConst, 0}, // 10.0
            {OpCode::PushConst, 1}, // 20.0
            {OpCode::Add, 0},
            {OpCode::Ret, 0}
        };
        code.constants = { 10.0, 20.0 };

        ExpressionContext ctx;
        double result = ExpressionVM::execute(code, ctx);
        check_near(result, 30.0, "Basic Add: 10 + 20");
    }

    // 2. Variables: x * 5
    {
        Bytecode code;
        code.instructions = {
            {OpCode::PushVar, 0},   // "x"
            {OpCode::PushConst, 0}, // 5.0
            {OpCode::Mul, 0},
            {OpCode::Ret, 0}
        };
        code.constants = { 5.0 };
        code.names = { "x" };

        ExpressionContext ctx;
        ctx.variables["x"] = 7.0;
        double result = ExpressionVM::execute(code, ctx);
        check_near(result, 35.0, "Variables: 7 * 5");
    }

    // 3. Complex expression: (a + b) * c - d / e
    // (10 + 5) * 2 - 8 / 4 = 15 * 2 - 2 = 30 - 2 = 28
    {
        Bytecode code;
        code.instructions = {
            {OpCode::PushConst, 0}, // 10
            {OpCode::PushConst, 1}, // 5
            {OpCode::Add, 0},       // 15
            {OpCode::PushConst, 2}, // 2
            {OpCode::Mul, 0},       // 30
            {OpCode::PushConst, 3}, // 8
            {OpCode::PushConst, 4}, // 4
            {OpCode::Div, 0},       // 2
            {OpCode::Sub, 0},       // 28
            {OpCode::Ret, 0}
        };
        code.constants = { 10.0, 5.0, 2.0, 8.0, 4.0 };

        ExpressionContext ctx;
        double result = ExpressionVM::execute(code, ctx);
        check_near(result, 28.0, "Complex: (10 + 5) * 2 - 8 / 4");
    }

    // 4. valueAtTime callback
    {
        Bytecode code;
        code.instructions = {
            {OpCode::PushConst, 0},
            {OpCode::Call, (1u << 16) | 0u}, // valueAtTime(0.25)
            {OpCode::Ret, 0}
        };
        code.constants = { 0.25 };
        code.names = { "valueAtTime" };

        ExpressionContext ctx;
        ctx.value = 9.0;
        ctx.value_at_time = [](double time) {
            return time * 4.0;
        };

        double result = ExpressionVM::execute(code, ctx);
        check_near(result, 1.0, "valueAtTime callback");
    }

    return g_failures == 0;
}
