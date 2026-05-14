#include "test_utils.h"
#include "tachyon/core/expressions/expression_engine.h"
#include <iostream>
#include <cmath>

namespace tachyon::expressions {

bool test_simple_expression() {
    ExpressionContext context;
    context.time = 1.0;
    
    auto result = CoreExpressionEvaluator::evaluate("sin(time * 2)", context);
    if (!result.success) {
        std::cerr << "Expression failed: " << result.error << "\n";
        return false;
    }

    double expected = std::sin(2.0);
    if (std::abs(result.value - expected) > 1e-6) {
        std::cerr << "Value mismatch: got " << result.value << ", expected " << expected << "\n";
        return false;
    }

    std::cout << "Expression test passed: sin(1.0 * 2) = " << result.value << "\n";
    return true;
}

} // namespace tachyon::expressions

int main() {
    if (tachyon::expressions::test_simple_expression()) {
        return 0;
    }
    return 1;
}
