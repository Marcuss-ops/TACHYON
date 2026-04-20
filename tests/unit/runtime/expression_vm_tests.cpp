#include "tachyon/runtime/vm/expression_vm.h"
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
    using namespace tachyon;

    // 1. Test Interpolate / Lerp
    {
        CompiledExpression expr;
        expr.code = {
            {ExpressionOp::PushConst, 0}, // a
            {ExpressionOp::PushConst, 1}, // b
            {ExpressionOp::PushConst, 2}, // t
            {ExpressionOp::Interpolate, 0},
            {ExpressionOp::Return, 0}
        };
        expr.constants = { 10.0, 20.0, 0.5 };

        ExpressionContext ctx;
        double result = ExpressionVM::evaluate(expr, ctx);
        check_near(result, 15.0, "Interpolate(10, 20, 0.5)");
    }

    // 2. Test Spring (Underdamped)
    {
        CompiledExpression expr;
        // Spring(from=100, to=0, freq=2, damping=0.5)
        expr.code = {
            {ExpressionOp::PushConst, 0}, // from
            {ExpressionOp::PushConst, 1}, // to
            {ExpressionOp::PushConst, 2}, // freq
            {ExpressionOp::PushConst, 3}, // damping
            {ExpressionOp::Spring, 0},
            {ExpressionOp::Return, 0}
        };
        expr.constants = { 100.0, 0.0, 2.0, 0.5 };

        ExpressionContext ctx;
        ctx.time_seconds = 0.0;
        check_near(ExpressionVM::evaluate(expr, ctx), 100.0, "Spring start value");

        ctx.time_seconds = 10.0; // Long time after
        check_near(ExpressionVM::evaluate(expr, ctx), 0.0, "Spring settled value", 0.1);
    }

    // 3. Test Spring (Critically Damped)
    {
        CompiledExpression expr;
        expr.code = {
            {ExpressionOp::PushConst, 0}, // from
            {ExpressionOp::PushConst, 1}, // to
            {ExpressionOp::PushConst, 2}, // freq
            {ExpressionOp::PushConst, 3}, // damping
            {ExpressionOp::Spring, 0},
            {ExpressionOp::Return, 0}
        };
        expr.constants = { 100.0, 0.0, 5.0, 1.0 }; // damping=1.0

        ExpressionContext ctx;
        ctx.time_seconds = 0.5;
        double val = ExpressionVM::evaluate(expr, ctx);
        // At t=0.5, freq=5, omega_n = 5 * 2pi = 31.415
        // decay = exp(-31.415 * 0.5) = exp(-15.7) ~= 0
        check_near(val, 0.0, "Critically damped settles fast", 0.01);
    }

    return g_failures == 0;
}
