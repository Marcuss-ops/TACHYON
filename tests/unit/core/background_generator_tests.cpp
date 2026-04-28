#include <gtest/gtest.h>
#include "tachyon/background_generator.h"

using namespace tachyon;

TEST(BackgroundGeneratorTest, DefaultParams) {
    ShapeGridParams params;
    ProceduralSpec spec = GenerateShapeGridBackground(params);

    EXPECT_EQ(spec.kind, "grid");
    EXPECT_EQ(spec.shape, "square");
    EXPECT_FLOAT_EQ(*spec.angle.value, 0.0f); // "right" → 0°
}

TEST(BackgroundGeneratorTest, DirectionAngles) {
    auto make = [](const std::string& dir) {
        ShapeGridParams p;
        p.direction = dir;
        return *GenerateShapeGridBackground(p).angle.value;
    };

    EXPECT_FLOAT_EQ(make("right"),    0.0f);
    EXPECT_FLOAT_EQ(make("left"),   180.0f);
    EXPECT_FLOAT_EQ(make("up"),      90.0f);
    EXPECT_FLOAT_EQ(make("down"),   270.0f);
    EXPECT_FLOAT_EQ(make("diagonal"), 45.0f);
}

TEST(BackgroundGeneratorTest, ParamsPassthrough) {
    ShapeGridParams params;
    params.shape = "circle";
    params.spacing = 60.0f;
    params.border_width = 2.5f;
    params.speed = 3.0f;
    params.seed = 9999;

    ProceduralSpec spec = GenerateShapeGridBackground(params);

    EXPECT_EQ(spec.shape, "circle");
    EXPECT_FLOAT_EQ(*spec.spacing.value, 60.0f);
    EXPECT_FLOAT_EQ(*spec.border_width.value, 2.5f);
    EXPECT_FLOAT_EQ(*spec.speed.value, 3.0f);
    EXPECT_EQ(spec.seed, 9999u);
}
