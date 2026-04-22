#include "tachyon/runtime/execution/render_session.h"
#include "tachyon/runtime/execution/quality_policy.h"
#include "tachyon/core/spec/scene_spec.h"
#include "tachyon/core/spec/scene_compiler.h"

#include <gtest/gtest.h>

namespace tachyon {

namespace {

SceneSpec make_scene(std::int64_t width, std::int64_t height) {
    LayerSpec layer;
    layer.id = "bg";
    layer.type = "solid";
    layer.name = "Background";
    layer.opacity = 1.0;

    CompositionSpec composition;
    composition.id = "main";
    composition.name = "Main";
    composition.width = width;
    composition.height = height;
    composition.duration = 1.0;
    composition.frame_rate = {30, 1};
    composition.layers.push_back(layer);

    SceneSpec scene;
    scene.spec_version = "1.0";
    scene.project.id = "proj";
    scene.project.name = "Quality Tier";
    scene.compositions.push_back(std::move(composition));
    return scene;
}

RenderExecutionPlan make_execution_plan(std::int64_t width, std::int64_t height) {
    RenderPlan plan;
    plan.job_id = "job_1";
    plan.scene_ref = "scene.json";
    plan.composition_target = "main";
    plan.composition.id = "main";
    plan.composition.name = "Main";
    plan.composition.width = width;
    plan.composition.height = height;
    plan.composition.duration = 1.0;
    plan.composition.frame_rate = {30, 1};
    plan.output.destination.path = "";
    plan.output.profile.name = "png-seq";
    plan.output.profile.class_name = "image-sequence";
    plan.output.profile.container = "png";
    plan.output.profile.alpha_mode = "preserved";
    plan.output.profile.video.codec = "png";
    plan.output.profile.video.pixel_format = "rgba8";
    plan.output.profile.video.rate_control_mode = "fixed";
    plan.output.profile.color.space = "bt709";
    plan.output.profile.color.transfer = "srgb";
    plan.output.profile.color.range = "full";
    plan.frame_range = {0, 0};

    RenderExecutionPlan exec;
    exec.render_plan = plan;
    FrameRenderTask task;
    task.frame_number = 0;
    task.cache_key = {"quality-tier-test"};
    task.cacheable = false;
    exec.frame_tasks.push_back(task);
    return exec;
}

} // namespace

TEST(QualityTierTests, TiersProduceDifferentFramebufferSizes) {
    const SceneSpec scene = make_scene(160, 90);
    SceneCompiler compiler;
    const auto compiled = compiler.compile(scene);
    ASSERT_TRUE(compiled.ok());

    RenderSession session;
    const RenderExecutionPlan exec_plan = make_execution_plan(160, 90);

    auto run_tier = [&](const std::string& tier) {
        RenderExecutionPlan tier_plan = exec_plan;
        tier_plan.render_plan.quality_tier = tier;
        const RenderSessionResult result = session.render(scene, *compiled.value, tier_plan, "");
        ASSERT_FALSE(result.frames.empty());
        ASSERT_TRUE(result.frames[0].frame);
        return result.frames[0].frame;
    };

    auto draft_frame = run_tier("draft");
    EXPECT_EQ(draft_frame->width(), 80U);
    EXPECT_EQ(draft_frame->height(), 45U);

    auto high_frame = run_tier("high");
    EXPECT_EQ(high_frame->width(), 160U);
    EXPECT_EQ(high_frame->height(), 90U);
}

TEST(QualityTierTests, DraftCapsMotionBlur) {
    const auto draft_policy = make_quality_policy("draft");
    EXPECT_EQ(draft_policy.motion_blur_sample_cap, 1);
}

} // namespace tachyon
