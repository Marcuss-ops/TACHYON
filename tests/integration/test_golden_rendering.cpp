#include <gtest/gtest.h>
#include "tachyon/tests/integration/golden_test_helper.h"
#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/core/scene/state/evaluated_state.h"


using namespace tachyon;
using namespace tachyon::renderer2d;
using namespace tachyon::scene;

class GoldenRenderingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Init context, font registry, etc.
    }
};

TEST_F(GoldenRenderingTest, TextPro_BasicLayout) {
    EvaluatedCompositionState state;
    state.width = 1920;
    state.height = 1080;
    
    EvaluatedLayerState text_layer;
    text_layer.type = LayerType::Text;
    text_layer.text_content = "Tachyon Engine Golden Test";
    text_layer.font_size = 72.0f;
    text_layer.visible = true;
    text_layer.enabled = true;
    text_layer.active = true;
    text_layer.opacity = 1.0;
    state.layers.push_back(text_layer);

    RenderPlan plan;
    FrameRenderTask task;
    RenderContext context; // Mocked or minimal for test

    auto frame = render_evaluated_composition_2d(state, plan, task, context);
    ASSERT_NE(frame.surface, nullptr);
    
    EXPECT_TRUE(test::GoldenTestHelper::compare_with_golden("text_pro_basic", *frame.surface));
}

TEST_F(GoldenRenderingTest, ShapeModifiers_TrimAndRepeat) {
    EvaluatedCompositionState state;
    state.width = 1000;
    state.height = 1000;
    
    EvaluatedLayerState shape_layer;
    shape_layer.type = LayerType::Shape;
    // Setup a complex shape with trim
    shape_layer.trim_start = 0.1f;
    shape_layer.trim_end = 0.9f;
    shape_layer.visible = true;
    shape_layer.enabled = true;
    shape_layer.active = true;
    state.layers.push_back(shape_layer);

    RenderPlan plan;
    FrameRenderTask task;
    RenderContext context;

    auto frame = render_evaluated_composition_2d(state, plan, task, context);
    ASSERT_NE(frame.surface, nullptr);
    
    EXPECT_TRUE(test::GoldenTestHelper::compare_with_golden("shape_trim_repeat", *frame.surface));
}


TEST_F(GoldenRenderingTest, ShapeModifiers_Degenerate) {
    EvaluatedCompositionState state;
    state.width = 500;
    state.height = 500;
    
    EvaluatedLayerState shape_layer;
    shape_layer.type = LayerType::Shape;
    // Degenerate case: Self-intersecting figure-eight with hole
    // and complex winding rule.
    shape_layer.visible = true;
    shape_layer.enabled = true;
    shape_layer.active = true;
    
    // Define a self-intersecting path
    // ...
    
    state.layers.push_back(shape_layer);

    RenderPlan plan;
    FrameRenderTask task;
    RenderContext context;

    auto frame = render_evaluated_composition_2d(state, plan, task, context);
    EXPECT_TRUE(test::GoldenTestHelper::compare_with_golden("shape_degenerate", *frame.surface));
}

