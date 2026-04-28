#include <gtest/gtest.h>
#include "tachyon/background_generator.h"

using namespace tachyon;

TEST(BackgroundGeneratorTest, GenerateNoise) {
    SceneSpec scene = BackgroundGenerator::GenerateNoiseBackground(1280, 720, 10.0, 2.5, 40.0, 12345);
    
    ASSERT_EQ(scene.compositions.size(), 1);
    const auto& comp = scene.compositions[0];
    EXPECT_EQ(comp.width, 1280);
    EXPECT_EQ(comp.height, 720);
    EXPECT_EQ(comp.duration, 10.0);
    
    ASSERT_EQ(comp.component_instances.size(), 1);
    const auto& inst = comp.component_instances[0];
    EXPECT_EQ(inst.component_id, "bg_noise");
    EXPECT_EQ(inst.param_values.at("frequency"), "2.500000");
    EXPECT_EQ(inst.param_values.at("amplitude"), "40.000000");
    EXPECT_EQ(inst.param_values.at("seed"), "12345");
}

TEST(BackgroundGeneratorTest, GenerateAura) {
    ColorSpec primary{255, 0, 0, 255};
    ColorSpec secondary{0, 255, 0, 255};
    SceneSpec scene = BackgroundGenerator::GenerateAuraBackground(1920, 1080, 5.0, primary, secondary, 0.8, 1.5);
    
    ASSERT_EQ(scene.compositions.size(), 1);
    const auto& comp = scene.compositions[0];
    
    ASSERT_EQ(comp.component_instances.size(), 1);
    const auto& inst = comp.component_instances[0];
    EXPECT_EQ(inst.component_id, "bg_aura_modern");
    EXPECT_EQ(inst.param_values.at("color_primary"), "#ff0000");
    EXPECT_EQ(inst.param_values.at("color_secondary"), "#00ff00");
    EXPECT_EQ(inst.param_values.at("speed"), "0.800000");
    EXPECT_EQ(inst.param_values.at("intensity"), "1.500000");
}

TEST(BackgroundGeneratorTest, GenerateFromComponent) {
    std::map<std::string, std::string> params = {{"custom_param", "custom_value"}};
    SceneSpec scene = BackgroundGenerator::GenerateFromComponent("custom_bg", params, 800, 600, 2.0);
    
    ASSERT_EQ(scene.compositions.size(), 1);
    const auto& comp = scene.compositions[0];
    EXPECT_EQ(comp.width, 800);
    EXPECT_EQ(comp.height, 600);
    
    ASSERT_EQ(comp.component_instances.size(), 1);
    const auto& inst = comp.component_instances[0];
    EXPECT_EQ(inst.component_id, "custom_bg");
    EXPECT_EQ(inst.param_values.at("custom_param"), "custom_value");
}
