#include "tachyon/text/animation/text_preset_registry.h"
#include "tachyon/text/animation/text_scene_presets.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cstddef>

using namespace tachyon;
using namespace tachyon::text;

class TextPresetContractTest : public ::testing::Test {
protected:
    void SetUp() override {
        registry = &TextPresetRegistry::instance();
    }
    TextPresetRegistry* registry;
};

// Contract: Registry should have presets registered
TEST_F(TextPresetContractTest, RegistryIsNotEmpty) {
    EXPECT_GT(registry->count(), 0u);
}

// Contract: Common presets should be available by ID
TEST_F(TextPresetContractTest, CommonPresetsAvailable) {
    EXPECT_NE(registry->find("fade_in"), nullptr);
    EXPECT_NE(registry->find("slide_in"), nullptr);
    EXPECT_NE(registry->find("pop_in"), nullptr);
    EXPECT_NE(registry->find("typewriter"), nullptr);
    EXPECT_NE(registry->find("blur_to_focus"), nullptr);
}

// Contract: Created preset has valid animator spec
TEST_F(TextPresetContractTest, CreatePresetReturnsValidSpec) {
    TextAnimatorSpec spec = registry->create("fade_in", "characters_excluding_spaces", 0.03, 0.7);
    EXPECT_FALSE(spec.selector.based_on.empty());
    EXPECT_GT(spec.properties.opacity_keyframes.size(), 0u);
}

// Contract: Scene generated has correct dimensions
TEST_F(TextPresetContractTest, SceneHasCorrectDimensions) {
    TextScenePresetOptions options;
    options.width = 1920;
    options.height = 1080;
    options.duration_seconds = 5.0;

    SceneSpec scene = make_enhance_text_scene(options);
    ASSERT_GT(scene.compositions.size(), 0u);
    
    const CompositionSpec& comp = scene.compositions[0];
    EXPECT_EQ(comp.width, 1920);
    EXPECT_EQ(comp.height, 1080);
    EXPECT_DOUBLE_EQ(comp.duration, 5.0);
}

// Contract: Scene has background layer when requested
TEST_F(TextPresetContractTest, SceneHasBackgroundLayer) {
    TextScenePresetOptions options;
    options.use_procedural_background = true;

    SceneSpec scene = make_enhance_text_scene(options);
    ASSERT_GT(scene.compositions.size(), 0u);
    
    const CompositionSpec& comp = scene.compositions[0];
    // First layer should be background
    ASSERT_GT(comp.layers.size(), 0u);
    EXPECT_EQ(comp.layers[0].type, "procedural");
    EXPECT_TRUE(comp.layers[0].procedural.has_value());
}

// Contract: Scene has text layer
TEST_F(TextPresetContractTest, SceneHasTextLayer) {
    TextScenePresetOptions options;

    SceneSpec scene = make_enhance_text_scene(options);
    ASSERT_GT(scene.compositions.size(), 0u);
    
    const CompositionSpec& comp = scene.compositions[0];
    // Should have at least 2 layers (background + text)
    ASSERT_GE(comp.layers.size(), 2u);
    
    // Last layer should be text
    const LayerSpec& text_layer = comp.layers.back();
    EXPECT_EQ(text_layer.type, "text");
    EXPECT_FALSE(text_layer.text_content.empty());
}

// Contract: Text layer has animators
TEST_F(TextPresetContractTest, TextLayerHasAnimators) {
    TextScenePresetOptions options;

    SceneSpec scene = make_enhance_text_scene(options);
    ASSERT_GT(scene.compositions.size(), 0u);
    
    const CompositionSpec& comp = scene.compositions[0];
    ASSERT_GE(comp.layers.size(), 2u);
    
    const LayerSpec& text_layer = comp.layers.back();
    EXPECT_GT(text_layer.text_animators.size(), 0u);
}

// Contract: Scene frame range is correct
TEST_F(TextPresetContractTest, SceneFrameRangeCorrect) {
    TextScenePresetOptions options;
    options.duration_seconds = 5.0;
    options.frame_rate = FrameRate(30, 1);

    SceneSpec scene = make_enhance_text_scene(options);
    ASSERT_GT(scene.compositions.size(), 0u);
    
    const CompositionSpec& comp = scene.compositions[0];
    double expected_frames = 5.0 * 30.0;
    
    // Check layers have correct in/out points
    for (const auto& layer : comp.layers) {
        EXPECT_DOUBLE_EQ(layer.in_point, 0.0);
        EXPECT_DOUBLE_EQ(layer.out_point, 5.0);
    }
}

// Contract: ShapeGrid background can be used
TEST_F(TextPresetContractTest, ShapeGridBackgroundWorks) {
    TextScenePresetOptions options;
    options.use_procedural_background = true;
    options.procedural_kind = "grid";
    options.shape_grid_params.shape = "square";
    options.shape_grid_params.spacing = 50.0f;

    SceneSpec scene = make_enhance_text_scene(options);
    ASSERT_GT(scene.compositions.size(), 0u);
    
    const CompositionSpec& comp = scene.compositions[0];
    ASSERT_GT(comp.layers.size(), 0u);
    
    const LayerSpec& bg_layer = comp.layers[0];
    ASSERT_TRUE(bg_layer.procedural.has_value());
    EXPECT_EQ(bg_layer.procedural->kind, "grid");
}

// Contract: Preset create with different parameters
TEST_F(TextPresetContractTest, PresetWithCustomParameters) {
    TextAnimatorSpec spec = registry->create("slide_in", "words", 0.05, 0.8);
    EXPECT_EQ(spec.selector.based_on, "words");
    EXPECT_GT(spec.properties.position_offset_keyframes.size(), 0u);
}

// Contract: Fallback for unknown preset ID
TEST_F(TextPresetContractTest, UnknownPresetFallsBack) {
    TextAnimatorSpec spec = registry->create("nonexistent_preset");
    // Should return a valid spec (fallback to fade_in)
    EXPECT_FALSE(spec.selector.based_on.empty());
    EXPECT_GT(spec.properties.opacity_keyframes.size(), 0u);
}
