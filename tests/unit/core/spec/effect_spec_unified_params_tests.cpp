#include "tachyon/core/spec/schema/effects/effect_spec.h"
#include <gtest/gtest.h>

namespace tachyon::test {

TEST(EffectSpecTest, AnimatedParams) {
    EffectSpec fx;
    fx.type = "tachyon.effect.blur";

    AnimatedScalarSpec radius;
    radius.keyframes.push_back({0.0, 0.0});
    radius.keyframes.push_back({1.0, 100.0});

    fx.set_param("radius", radius);

    auto a = fx.evaluate(0.0);
    auto b = fx.evaluate(0.5);
    auto c = fx.evaluate(1.0);

    EXPECT_EQ(std::get<float>(a.params.at("radius")), 0.0f);
    EXPECT_NEAR(std::get<float>(b.params.at("radius")), 50.0f, 0.1f);
    EXPECT_EQ(std::get<float>(c.params.at("radius")), 100.0f);
}

TEST(EffectSpecTest, ExpressionParams) {
    EffectSpec fx;
    fx.type = "tachyon.effect.transition.glsl";

    AnimatedScalarSpec progress;
    progress.expression = "t / 3.0";

    fx.set_param("progress", progress);
    
    // Note: evaluate() needs to support expressions. 
    // If not implemented, this test documented the gap.
    auto eval = fx.evaluate(1.5);
    EXPECT_NEAR(std::get<float>(eval.params.at("progress")), 0.5f, 0.01f);
}

} // namespace tachyon::test
