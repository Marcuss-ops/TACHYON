#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/planning/quality_policy.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/compilation/scene_compiler.h"

#include <gtest/gtest.h>

namespace tachyon {

namespace {

SceneSpec make_scene(std::int64_t width, std::int64_t height) {
    LayerSpec layer;
    layer.id = "bg";
    layer.type = LayerType::Solid;
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
    scene.project.id = "proj";
    scene.project.name = "Quality Tier";
    scene.compositions.push_back(std::move(composition));
    return scene;
}

RenderExecutionPlan make_execution_plan(std::int64_t width, std::int64_t height) {
    RenderPlan plan;
    plan.job_id = "job_1";
    plan.scene_ref = "scene_builder";
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

TEST(QualityTierTests, EnumFactoryIsDeterministic) {
    const auto draft = make_quality_policy(QualityTier::Draft);
    const auto preview = make_quality_policy(QualityTier::Preview);
    const auto production = make_quality_policy(QualityTier::Production);
    const auto cinematic = make_quality_policy(QualityTier::Cinematic);

    EXPECT_EQ(draft.resolution_scale, 0.5f);
    EXPECT_EQ(preview.resolution_scale, 0.75f);
    EXPECT_EQ(production.resolution_scale, 1.0f);
    EXPECT_EQ(cinematic.resolution_scale, 1.0f);

    EXPECT_EQ(draft.motion_blur_samples, 1);
    EXPECT_EQ(preview.motion_blur_samples, 4);
    EXPECT_EQ(production.motion_blur_samples, 8);
    EXPECT_EQ(cinematic.motion_blur_samples, 16);
}

TEST(QualityTierTests, StringFactoryAcceptsAllTiers) {
    bool threw = false;
    try { make_quality_policy("draft"); } catch (...) { threw = true; }
    EXPECT_FALSE(threw);
    try { make_quality_policy("preview"); } catch (...) { threw = true; }
    EXPECT_FALSE(threw);
    try { make_quality_policy("production"); } catch (...) { threw = true; }
    EXPECT_FALSE(threw);
    try { make_quality_policy("cinematic"); } catch (...) { threw = true; }
    EXPECT_FALSE(threw);
    try { make_quality_policy("high"); } catch (...) { threw = true; }
    EXPECT_FALSE(threw); // backward-compat alias
}

TEST(QualityTierTests, StringFactoryIsDeterministic) {
    const auto p1 = make_quality_policy("production");
    const auto p2 = make_quality_policy("production");
    EXPECT_EQ(p1.resolution_scale, p2.resolution_scale);
    EXPECT_EQ(p1.dof_sample_count, p2.dof_sample_count);
    EXPECT_EQ(p1.ray_tracer_max_bounces, p2.ray_tracer_max_bounces);
}

TEST(QualityTierTests, DoFParametersDifferByTier) {
    const auto draft = make_quality_policy(QualityTier::Draft);
    const auto preview = make_quality_policy(QualityTier::Preview);
    const auto production = make_quality_policy(QualityTier::Production);
    const auto cinematic = make_quality_policy(QualityTier::Cinematic);

    // max_coc_radius_px increases with quality
    EXPECT_LT(draft.max_coc_radius_px, preview.max_coc_radius_px);
    EXPECT_LT(preview.max_coc_radius_px, production.max_coc_radius_px);
    EXPECT_LT(production.max_coc_radius_px, cinematic.max_coc_radius_px);

    // dof_sample_count increases with quality
    EXPECT_LT(draft.dof_sample_count, preview.dof_sample_count);
    EXPECT_LT(preview.dof_sample_count, production.dof_sample_count);
    EXPECT_LT(production.dof_sample_count, cinematic.dof_sample_count);

    // fg/bg separation enabled only at production and above
    EXPECT_FALSE(draft.dof_fg_bg_separate);
    EXPECT_FALSE(preview.dof_fg_bg_separate);
    EXPECT_TRUE(production.dof_fg_bg_separate);
    EXPECT_TRUE(cinematic.dof_fg_bg_separate);
}

TEST(QualityTierTests, RayTracingParametersDifferByTier) {
    const auto draft = make_quality_policy(QualityTier::Draft);
    const auto preview = make_quality_policy(QualityTier::Preview);
    const auto production = make_quality_policy(QualityTier::Production);
    const auto cinematic = make_quality_policy(QualityTier::Cinematic);

    EXPECT_EQ(draft.ray_tracer_spp, 1);
    EXPECT_EQ(preview.ray_tracer_spp, 1);
    EXPECT_EQ(production.ray_tracer_spp, 4);
    EXPECT_EQ(cinematic.ray_tracer_spp, 16);

    EXPECT_EQ(draft.ray_tracer_max_bounces, 1);
    EXPECT_EQ(preview.ray_tracer_max_bounces, 2);
    EXPECT_EQ(production.ray_tracer_max_bounces, 4);
    EXPECT_EQ(cinematic.ray_tracer_max_bounces, 8);

    EXPECT_FALSE(draft.denoiser_enabled);
    EXPECT_FALSE(preview.denoiser_enabled);
    EXPECT_TRUE(production.denoiser_enabled);
    EXPECT_TRUE(cinematic.denoiser_enabled);
}

TEST(QualityTierTests, ShadowParametersDifferByTier) {
    const auto draft = make_quality_policy(QualityTier::Draft);
    const auto preview = make_quality_policy(QualityTier::Preview);
    const auto production = make_quality_policy(QualityTier::Production);

    EXPECT_EQ(draft.shadow_map_resolution, 256);
    EXPECT_EQ(preview.shadow_map_resolution, 512);
    EXPECT_EQ(production.shadow_map_resolution, 1024);

    EXPECT_FALSE(draft.soft_shadows);
    EXPECT_FALSE(preview.soft_shadows);
    EXPECT_TRUE(production.soft_shadows);
}

TEST(QualityTierTests, InvalidTierThrows) {
    bool threw = false;
    try { make_quality_policy("ultra"); } catch (const std::invalid_argument&) { threw = true; }
    EXPECT_TRUE(threw);
    threw = false;
    try { make_quality_policy(""); } catch (const std::invalid_argument&) { threw = true; }
    EXPECT_TRUE(threw);
    threw = false;
    try { make_quality_policy("low"); } catch (const std::invalid_argument&) { threw = true; }
    EXPECT_TRUE(threw);
}

TEST(QualityTierTests, RoundTripStringConversion) {
    EXPECT_EQ(std::string(to_string(QualityTier::Draft)), "draft");
    EXPECT_EQ(std::string(to_string(QualityTier::Preview)), "preview");
    EXPECT_EQ(std::string(to_string(QualityTier::Production)), "production");
    EXPECT_EQ(std::string(to_string(QualityTier::Cinematic)), "cinematic");

    EXPECT_EQ(quality_tier_from_string("draft"), QualityTier::Draft);
    EXPECT_EQ(quality_tier_from_string("preview"), QualityTier::Preview);
    EXPECT_EQ(quality_tier_from_string("production"), QualityTier::Production);
    EXPECT_EQ(quality_tier_from_string("cinematic"), QualityTier::Cinematic);
    EXPECT_EQ(quality_tier_from_string("high"), QualityTier::Production);
}

} // namespace tachyon
