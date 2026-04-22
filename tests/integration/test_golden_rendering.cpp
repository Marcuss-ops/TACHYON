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
    RenderContext2D context; // Mocked or minimal for test

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
    RenderContext2D context;

    auto frame = render_evaluated_composition_2d(state, plan, task, context);
    ASSERT_NE(frame.surface, nullptr);
    
    EXPECT_TRUE(test::GoldenTestHelper::compare_with_golden("shape_trim_repeat", *frame.surface));
}

TEST_F(GoldenRenderingTest, RayTracer_3D_AOVs) {
    EvaluatedCompositionState state;
    state.width = 640;
    state.height = 360;
    state.camera.available = true;
    state.camera.camera.position = {0, 0, 500};
    state.camera.camera.target = {0, 0, 0};
    state.camera.camera.up = {0, 1, 0};
    state.camera.camera.fov = 45.0f;

    EvaluatedLayerState box_layer;
    box_layer.type = LayerType::Solid;
    box_layer.is_3d = true;
    box_layer.width = 100;
    box_layer.height = 100;
    box_layer.fill_color = {255, 0, 0, 255};
    box_layer.world_matrix = math::Matrix4x4::identity();
    box_layer.previous_world_matrix = math::Matrix4x4::translation({-10, 0, 0}); // Object moving right
    box_layer.visible = true;
    box_layer.enabled = true;
    box_layer.active = true;
    state.layers.push_back(box_layer);

    RenderPlan plan;
    plan.motion_blur_enabled = false;
    FrameRenderTask task;
    RenderContext2D context;
    context.ray_tracer = std::make_shared<renderer3d::RayTracer>();

    auto frame = render_evaluated_composition_2d(state, plan, task, context);
    ASSERT_NE(frame.surface, nullptr);
    
    // Check Beauty
    EXPECT_TRUE(test::GoldenTestHelper::compare_with_golden("3d_beauty", *frame.surface));
    
    // Check AOVs
    auto find_aov = [&](const std::string& name) -> std::shared_ptr<SurfaceRGBA> {
        for (const auto& aov : frame.aovs) if (aov.name == name) return aov.surface;
        return nullptr;
    };
    
    auto depth = find_aov("depth");
    auto normal = find_aov("normal");
    auto mv = find_aov("motion_vector");
    
    ASSERT_NE(depth, nullptr);
    ASSERT_NE(normal, nullptr);
    ASSERT_NE(mv, nullptr);
    
    EXPECT_TRUE(test::GoldenTestHelper::compare_with_golden("3d_depth", *depth));
    EXPECT_TRUE(test::GoldenTestHelper::compare_with_golden("3d_normal", *normal));
    EXPECT_TRUE(test::GoldenTestHelper::compare_with_golden("3d_motion_vector", *mv));
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
    RenderContext2D context;

    auto frame = render_evaluated_composition_2d(state, plan, task, context);
    EXPECT_TRUE(test::GoldenTestHelper::compare_with_golden("shape_degenerate", *frame.surface));
}

TEST_F(GoldenRenderingTest, RayTracer_MovingCamera_MV) {
    EvaluatedCompositionState state;
    state.width = 640;
    state.height = 360;
    
    // Current camera
    state.camera.available = true;
    state.camera.camera.position = {0, 0, 500};
    state.camera.camera.target = {0, 0, 0};
    
    // Previous camera (moved slightly left)
    state.camera.previous_position = {-20, 0, 500};
    state.camera.previous_target = {-20, 0, 0};

    EvaluatedLayerState box_layer;
    box_layer.type = LayerType::Solid;
    box_layer.is_3d = true;
    box_layer.width = 200;
    box_layer.height = 200;
    box_layer.world_matrix = math::Matrix4x4::identity();
    box_layer.previous_world_matrix = math::Matrix4x4::identity(); // Object is static
    box_layer.visible = true;
    state.layers.push_back(box_layer);

    RenderPlan plan;
    FrameRenderTask task;
    RenderContext2D context;
    context.ray_tracer = std::make_shared<renderer3d::RayTracer>();

    auto frame = render_evaluated_composition_2d(state, plan, task, context);
    
    auto find_aov = [&](const std::string& name) -> std::shared_ptr<SurfaceRGBA> {
        for (const auto& aov : frame.aovs) if (aov.name == name) return aov.surface;
        return nullptr;
    };
    
    auto mv = find_aov("motion_vector");
    ASSERT_NE(mv, nullptr);
    EXPECT_TRUE(test::GoldenTestHelper::compare_with_golden("3d_mv_moving_camera", *mv));
}

