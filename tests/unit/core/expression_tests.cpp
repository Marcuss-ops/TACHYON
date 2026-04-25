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

    // 2. AE-style contract variables
    {
        ExpressionContext ae_ctx;
        ae_ctx.time = 1.5;
        ae_ctx.composition_time = 2.0;
        ae_ctx.composition_width = 1920;
        ae_ctx.composition_height = 1080;
        ae_ctx.layer_index = 4;
        ae_ctx.property_index = 6;
        ae_ctx.value = 12.5;
        ae_ctx.seed = 7;
        ae_ctx.value_at_time = [](double sample_time) {
            return sample_time * 8.0;
        };

        auto result = ExpressionEvaluator::evaluate("t + time + value + seed + index", ae_ctx);
        check_near(result.value, 1.5 + 1.5 + 12.5 + 7.0 + 4.0, "AE variables: t/time/value/seed/index");

        result = ExpressionEvaluator::evaluate("thisComp.width + thisComp.height", ae_ctx);
        check_near(result.value, 3000.0, "AE variables: thisComp.width + thisComp.height");

        result = ExpressionEvaluator::evaluate("thisComp.time", ae_ctx);
        check_near(result.value, 2.0, "AE variables: thisComp.time");

        result = ExpressionEvaluator::evaluate("thisLayer.index", ae_ctx);
        check_near(result.value, 4.0, "AE variables: thisLayer.index");

        result = ExpressionEvaluator::evaluate("thisProperty.index", ae_ctx);
        check_near(result.value, 6.0, "AE variables: thisProperty.index");

        result = ExpressionEvaluator::evaluate("valueAtTime(0.25)", ae_ctx);
        check_near(result.value, 2.0, "AE built-in: valueAtTime(0.25)");
    }

    // 3. Power and Unary
    {
        auto result = ExpressionEvaluator::evaluate("2 ^ 3", ctx);
        check_near(result.value, 8.0, "Power: 2 ^ 3");

        result = ExpressionEvaluator::evaluate("-5 + 10", ctx);
        check_near(result.value, 5.0, "Unary minus: -5 + 10");
        
        result = ExpressionEvaluator::evaluate("2 * -3", ctx);
        check_near(result.value, -6.0, "Unary minus in expression: 2 * -3");
    }

    // 4. Variables
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

    // 5. Functions
    {
        auto result = ExpressionEvaluator::evaluate("sin(0)", ctx);
        check_near(result.value, 0.0, "Function: sin(0)");

        result = ExpressionEvaluator::evaluate("abs(-5.5)", ctx);
        check_near(result.value, 5.5, "Function: abs(-5.5)");

        result = ExpressionEvaluator::evaluate("clamp(50, 0, 10)", ctx);
        check_near(result.value, 10.0, "Function: clamp(50, 0, 10)");
    }

    // 6. ExpressionProperty Integration
    {
        ExpressionProperty<float> prop("osc", "sin(t * 1.570796)"); // at t=1, sin(pi/2) = 1
        tachyon::properties::PropertyEvaluationContext eval_ctx;
        eval_ctx.time = 1.0;
        eval_ctx.seed = 99;
        eval_ctx.value = 12.0;
        eval_ctx.composition_width = 1280;
        eval_ctx.composition_height = 720;
        eval_ctx.layer_index = 3;
        eval_ctx.value_at_time = [](double sample_time) {
            return sample_time + 5.0;
        };
        
        // Let's also test the evaluator directly with the same string
        ExpressionContext direct_ctx;
        direct_ctx.variables["t"] = 1.0;
        auto direct_res = ExpressionEvaluator::evaluate("sin(t * 1.570796)", direct_ctx);
        if (!direct_res.success) {
            std::cerr << "Evaluator Error: " << direct_res.error << '\n';
        }

        float val = prop.sample(eval_ctx);
        check_near(val, 1.0, "ExpressionProperty evaluation at t=1.0");

        ExpressionProperty<float> seeded_prop("seeded", "seed + t");
        check_near(seeded_prop.sample(eval_ctx), 100.0, "ExpressionProperty exposes deterministic seed");

        ExpressionProperty<float> width_prop("width", "thisComp.width / 2");
        check_near(width_prop.sample(eval_ctx), 640.0, "ExpressionProperty exposes thisComp.width");

        ExpressionProperty<float> sample_prop("sample", "valueAtTime(0.5)");
        check_near(sample_prop.sample(eval_ctx), 5.5, "ExpressionProperty exposes valueAtTime()");

        ExpressionProperty<float> index_prop("index", "thisProperty.index");
        check_near(index_prop.sample(eval_ctx), 0.0, "ExpressionProperty exposes thisProperty.index default");
    }

    // 7. New Built-ins
    {
        ctx.variables["t"] = 2.5;
        ctx.layer_index = 3;
        
        auto result = ExpressionEvaluator::evaluate("pingpong(t, 2.0)", ctx);
        check_near(result.value, 1.5, "Function: pingpong(2.5, 2.0)");

        result = ExpressionEvaluator::evaluate("timeStretch(t, 0.5)", ctx);
        check_near(result.value, 1.25, "Function: timeStretch(2.5, 0.5)");

        result = ExpressionEvaluator::evaluate("stagger(10.0)", ctx);
        check_near(result.value, 30.0, "Function: stagger(10.0) with index 3");

        result = ExpressionEvaluator::evaluate("lerp(10, 20, 0.5)", ctx);
        check_near(result.value, 15.0, "Function: lerp(10, 20, 0.5)");

        result = ExpressionEvaluator::evaluate("interpolate(10, 20, 0.8)", ctx);
        check_near(result.value, 18.0, "Function: interpolate(10, 20, 0.8)");

        // spring(t, from, to, freq, damping)
        result = ExpressionEvaluator::evaluate("spring(0.1, 0, 100, 5, 0.5)", ctx);
        check_true(result.value > 0.0, "Function: spring() produced non-zero result");

        // global context variables: index, value
        ctx.layer_index = 7;
        ctx.value = 42.5;
        result = ExpressionEvaluator::evaluate("index * 10", ctx);
        check_near(result.value, 70.0, "Global variable: index (7)");

        result = ExpressionEvaluator::evaluate("value + 5", ctx);
        check_near(result.value, 47.5, "Global variable: value (42.5)");
    }

    return g_failures == 0;
}
