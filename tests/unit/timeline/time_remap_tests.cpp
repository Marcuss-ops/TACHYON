#include "tachyon/timeline/time_remap.h"
#include "tachyon/core/spec/schema/contracts/shared_contracts.h"
#include <gtest/gtest.h>
#include <vector>
#include <cmath>

using namespace tachyon::timeline;
using namespace tachyon::spec;

TEST(TimeRemapTest, EvaluateSourceTimeLinear) {
    TimeRemapCurve curve;
    curve.mode = TimeRemapMode::Linear;
    curve.keyframes = {{0.0f, 0.0f}, {10.0f, 10.0f}}; // 1:1 mapping

    EXPECT_FLOAT_EQ(evaluate_source_time(curve, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(evaluate_source_time(curve, 5.0f), 5.0f);
    EXPECT_FLOAT_EQ(evaluate_source_time(curve, 10.0f), 10.0f);
}

TEST(TimeRemapTest, EvaluateSourceTimeHold) {
    TimeRemapCurve curve;
    curve.mode = TimeRemapMode::Hold;
    curve.keyframes = {{0.0f, 0.0f}, {10.0f, 10.0f}};

    EXPECT_FLOAT_EQ(evaluate_source_time(curve, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(evaluate_source_time(curve, 5.0f), 0.0f); // Hold previous
    EXPECT_FLOAT_EQ(evaluate_source_time(curve, 10.0f), 10.0f);
}

TEST(TimeRemapTest, EvaluateSourceTimeRampUp) {
    // Dest 0->10 maps to Source 0->20 (speed x2)
    TimeRemapCurve curve;
    curve.mode = TimeRemapMode::Linear;
    curve.keyframes = {{0.0f, 0.0f}, {20.0f, 10.0f}};

    EXPECT_FLOAT_EQ(evaluate_source_time(curve, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(evaluate_source_time(curve, 5.0f), 10.0f);
    EXPECT_FLOAT_EQ(evaluate_source_time(curve, 10.0f), 20.0f);
}

TEST(TimeRemapTest, EvaluateSourceTimeRampDown) {
    // Dest 0->10 maps to Source 20->0 (reverse, speed -x2)
    TimeRemapCurve curve;
    curve.mode = TimeRemapMode::Linear;
    curve.keyframes = {{20.0f, 0.0f}, {0.0f, 10.0f}};

    EXPECT_FLOAT_EQ(evaluate_source_time(curve, 0.0f), 20.0f);
    EXPECT_FLOAT_EQ(evaluate_source_time(curve, 5.0f), 10.0f);
    EXPECT_FLOAT_EQ(evaluate_source_time(curve, 10.0f), 0.0f);
}

TEST(TimeRemapTest, EvaluateSourceTimeClamp) {
    TimeRemapCurve curve;
    curve.mode = TimeRemapMode::Linear;
    curve.keyframes = {{0.0f, 0.0f}, {10.0f, 10.0f}};

    // Dest < first keyframe dest time: clamp to first source time
    EXPECT_FLOAT_EQ(evaluate_source_time(curve, -5.0f), 0.0f);
    // Dest > last keyframe dest time: clamp to last source time
    EXPECT_FLOAT_EQ(evaluate_source_time(curve, 15.0f), 10.0f);
}

TEST(TimeRemapTest, EvaluateSourceTimeExtrapolation) {
    TimeRemapCurve curve;
    curve.mode = TimeRemapMode::Linear;
    curve.keyframes = {{0.0f, 0.0f}, {10.0f, 10.0f}};
    curve.extrapolation = spec::ExtrapolationMode::Extrapolate; // Assume this exists

    // Extrapolate beyond last keyframe
    EXPECT_FLOAT_EQ(evaluate_source_time(curve, 15.0f), 15.0f);
    // Extrapolate before first keyframe
    EXPECT_FLOAT_EQ(evaluate_source_time(curve, -5.0f), -5.0f);
}

TEST(TimeRemapTest, FrameBlendLinear) {
    TimeRemapCurve curve;
    curve.mode = TimeRemapMode::Linear;
    curve.keyframes = {{0.0f, 0.0f}, {10.0f, 10.0f}};

    FrameBlendResult result = evaluate_frame_blend(curve, 5.0f, 1.0f);
    EXPECT_FLOAT_EQ(result.source_time_a, 5.0f);
    EXPECT_NEAR(result.blend_factor, 0.0f, 1e-6f);
}

TEST(TimeRemapTest, TimeRemapEvaluatorWithFlow) {
    TimeRemapEvaluator::Config config;
    config.optical_flow_confidence_threshold = 0.5f;
    config.enable_optical_flow_warping = false;
    TimeRemapEvaluator evaluator(config);

    TimeRemapCurve curve;
    curve.mode = TimeRemapMode::Linear;
    curve.keyframes = {{0.0f, 0.0f}, {10.0f, 10.0f}};

    float source_time = evaluator.evaluate(curve, 5.0f, 1.0f);
    EXPECT_FLOAT_EQ(source_time, 5.0f);
}
