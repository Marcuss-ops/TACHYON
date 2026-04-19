#include "tachyon/core/properties/property.h"
#include "tachyon/core/animation/animatable.h"
#include "tachyon/core/math/vector3.h"

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

} // namespace

bool run_property_tests() {
    using namespace tachyon::properties;
    using namespace tachyon::math;

    // Test 1: Constant Property
    {
        ConstantProperty<float> opacity("Opacity", 0.75f);
        PropertyEvaluationContext ctx{ 5.0, 0 }; // Time 5.0
        
        float val = opacity.sample(ctx);
        check_true(std::abs(val - 0.75f) < 1e-6f, "Constant float property sample");
        check_true(opacity.get_name() == "Opacity", "Property name metadata");
    }

    // Test 2: Animation Curve (Baseline)
    {
        using namespace tachyon::animation;
        AnimationCurve<float> curve;
        curve.keyframes = {
            { 0.0, 10.0f },
            { 1.0, 20.0f }
        };

        float v0 = curve.evaluate(0.0);
        float v05 = curve.evaluate(0.5);
        float v1 = curve.evaluate(1.0);

        check_true(std::abs(v0 - 10.0f) < 1e-6f, "Curve evaluate at start");
        check_true(std::abs(v05 - 15.0f) < 1e-6f, "Curve evaluate at midpoint (linear)");
        check_true(std::abs(v1 - 20.0f) < 1e-6f, "Curve evaluate at end");
    }

    return g_failures == 0;
}
