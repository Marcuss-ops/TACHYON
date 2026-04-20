#include "tachyon/core/expressions/expression_engine.h"
#include "tachyon/core/properties/expression_property.h"
#include <iostream>
#include <string>
#include <cmath>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void check_near(double actual, double expected, const std::string& message, double epsilon = 0.0001) {
    if (std::abs(actual - expected) > epsilon) {
        ++g_failures;
        std::cerr << "FAIL: " << message << " (Expected " << expected << ", got " << actual << ")\n";
    }
}

} // namespace

bool run_expression_tests() {
    using namespace tachyon::expressions;
    using namespace tachyon::properties;

    ExpressionContext ctx;
    ctx.variables["t"] = 2.0;
    ctx.variables["val"] = 10.0;
    ctx.seed = 1234;

    // 1. Basic Arithmetic
    {
        auto result = ExpressionEvaluator::evaluate("2 + 3 * 4", ctx);
        check_near(result.value, 14.0, "Precedence: 2 + 3 * 4");

        result = ExpressionEvaluator::evaluate("(2 + 3) * 4", ctx);
        check_near(result.value, 20.0, "Parentheses: (2 + 3) * 4");

        result = ExpressionEvaluator::evaluate("10 / 2 - 1", ctx);
        check_near(result.value, 4.0, "Division: 10 / 2 - 1");
    }

    // 2. Power and Unary
    {
        auto result = ExpressionEvaluator::evaluate("2 ^ 3", ctx);
        check_near(result.value, 8.0, "Power: 2 ^ 3");

        result = ExpressionEvaluator::evaluate("-5 + 10", ctx);
        check_near(result.value, 5.0, "Unary minus: -5 + 10");
        
        result = ExpressionEvaluator::evaluate("2 * -3", ctx);
        check_near(result.value, -6.0, "Unary minus in expression: 2 * -3");
    }

    // 3. Variables
    {
        auto result = ExpressionEvaluator::evaluate("t * val", ctx);
        check_near(result.value, 20.0, "Variables: t * val");

        result = ExpressionEvaluator::evaluate("seed + t", ctx);
        check_near(result.value, 1236.0, "Variables: seed + t");

        ExpressionContext dotted_ctx;
        dotted_ctx.variables["music.bass"] = 3.0;
        result = ExpressionEvaluator::evaluate("music.bass + 2", dotted_ctx);
        check_near(result.value, 5.0, "Variables: music.bass + 2");
    }

    // 4. Functions
    {
        auto result = ExpressionEvaluator::evaluate("sin(0)", ctx);
        check_near(result.value, 0.0, "Function: sin(0)");

        result = ExpressionEvaluator::evaluate("abs(-5.5)", ctx);
        check_near(result.value, 5.5, "Function: abs(-5.5)");

        result = ExpressionEvaluator::evaluate("clamp(50, 0, 10)", ctx);
        check_near(result.value, 10.0, "Function: clamp(50, 0, 10)");
    }

    // 5. ExpressionProperty Integration
    {
        ExpressionProperty<float> prop("osc", "sin(t * 1.570796)"); // at t=1, sin(pi/2) = 1
        tachyon::properties::PropertyEvaluationContext eval_ctx;
        eval_ctx.time = 1.0;
        eval_ctx.seed = 99;
        
        // Let's also test the evaluator directly with the same string
        ExpressionContext direct_ctx;
        direct_ctx.variables["t"] = 1.0;
        auto direct_res = ExpressionEvaluator::evaluate("sin(t * 1.570796)", direct_ctx);
        if (!direct_res.success) {
            fprintf(stderr, "Evaluator Error: %s\n", direct_res.error.c_str());
        }

        float val = prop.sample(eval_ctx);
        if (val == 0.0f) {
             // Maybe it failed here too
             auto res2 = ExpressionEvaluator::evaluate("sin(t * 1.570796)", direct_ctx);
             fprintf(stderr, "Prop Eval Error: %s\n", res2.error.c_str());
        }
        check_near(val, 1.0, "ExpressionProperty evaluation at t=1.0");

        ExpressionProperty<float> seeded_prop("seeded", "seed + t");
        check_near(seeded_prop.sample(eval_ctx), 100.0, "ExpressionProperty exposes deterministic seed");
    }

    return g_failures == 0;
}
