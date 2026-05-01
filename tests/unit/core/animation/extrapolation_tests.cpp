#include <gtest/gtest.h>
#include "tachyon/core/animation/animation_curve.h"

using namespace tachyon::animation;

TEST(AnimationCurveExtrapolation, ClampMode) {
    AnimationCurve<double> curve;
    curve.add_keyframe({1.0, 10.0});
    curve.add_keyframe({2.0, 20.0});
    curve.sort();

    curve.pre_infinity = ExtrapolationMode::Clamp;
    curve.post_infinity = ExtrapolationMode::Clamp;

    EXPECT_NEAR(curve.evaluate(0.0), 10.0, 1e-6);
    EXPECT_NEAR(curve.evaluate(1.0), 10.0, 1e-6);
    EXPECT_NEAR(curve.evaluate(1.5), 15.0, 1e-6);
    EXPECT_NEAR(curve.evaluate(2.0), 20.0, 1e-6);
    EXPECT_NEAR(curve.evaluate(3.0), 20.0, 1e-6);
}

TEST(AnimationCurveExtrapolation, LoopMode) {
    AnimationCurve<double> curve;
    curve.add_keyframe({1.0, 10.0});
    curve.add_keyframe({2.0, 20.0});
    curve.sort();

    curve.pre_infinity = ExtrapolationMode::Loop;
    curve.post_infinity = ExtrapolationMode::Loop;

    // Range [1, 2], duration 1.0
    EXPECT_NEAR(curve.evaluate(2.5), 15.0, 1e-6); // 2.5 -> 1.5
    EXPECT_NEAR(curve.evaluate(3.0), 10.0, 1e-6); // 3.0 -> 2.0 (but Loop 2.0 is same as 1.0)
    EXPECT_NEAR(curve.evaluate(0.5), 15.0, 1e-6); // 0.5 -> 1.5
}

TEST(AnimationCurveExtrapolation, PingPongMode) {
    AnimationCurve<double> curve;
    curve.add_keyframe({1.0, 10.0});
    curve.add_keyframe({2.0, 20.0});
    curve.sort();

    curve.pre_infinity = ExtrapolationMode::PingPong;
    curve.post_infinity = ExtrapolationMode::PingPong;

    // Range [1, 2], duration 1.0. PingPong duration 2.0.
    EXPECT_NEAR(curve.evaluate(2.5), 15.0, 1e-6); // 2.5 -> 1.5
    EXPECT_NEAR(curve.evaluate(3.0), 10.0, 1e-6); // 3.0 -> 2.0 (back to start)
    EXPECT_NEAR(curve.evaluate(3.5), 15.0, 1e-6); // 3.5 -> 1.5
}

TEST(AnimationCurveExtrapolation, ExtendMode) {
    AnimationCurve<double> curve;
    curve.add_keyframe({1.0, 10.0});
    curve.add_keyframe({2.0, 20.0});
    curve.sort();

    curve.pre_infinity = ExtrapolationMode::Extend;
    curve.post_infinity = ExtrapolationMode::Extend;

    // Slope is (20-10)/(2-1) = 10 units/sec
    EXPECT_NEAR(curve.evaluate(3.0), 30.0, 1e-6); // 20 + 10*(3-2)
    EXPECT_NEAR(curve.evaluate(0.0), 0.0, 1e-6);  // 10 + 10*(0-1)
}
