#include <gtest/gtest.h>
#include "tachyon/presets/text/layer.h"
#include "tachyon/presets/scene/common.h"
#include "tachyon/presets/transition/transition_params.h"

using namespace tachyon::presets;

TEST(TextPresetTests, ParamsInheritance) {
    text::TextParams params;
    // Check if it inherits from LayerParams (implicitly)
    LayerParams* base = &params;
    EXPECT_NE(base, nullptr);
}

TEST(TextPresetTests, BuildEnhanced) {
    text::TextParams params;
    params.text = "Test Text";
    auto layer = text::build_enhanced(params);
    EXPECT_EQ(layer.text_content, "Test Text");
    EXPECT_EQ(layer.name, "Enhanced Text");
}

TEST(ScenePresetTests, BuildEnhanced) {
    scene::TextSceneParams params;
    params.text = "Scene Test";
    auto scene = scene::build_enhanced_text_scene(params);
    ASSERT_FALSE(scene.compositions.empty());
    EXPECT_EQ(scene.compositions[0].layers.back().text_content, "Scene Test");
}

TEST(TransitionPresetTests, ParamsInheritance) {
    TransitionParams params;
    LayerParams* base = &params;
    EXPECT_NE(base, nullptr);
}
