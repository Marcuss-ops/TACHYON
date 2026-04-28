#include <gtest/gtest.h>
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include "tachyon/core/animation/easing.h"

#include <cmath>
#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

tachyon::SceneSpec make_scene(double x_offset = 0.0) {
    tachyon::LayerSpec layer;
    layer.id = "solid_1";
    layer.type = "solid";
    layer.name = "Solid One";
    layer.start_time = 0.0;
    layer.in_point = 0.0;
    layer.out_point = 2.0;
    layer.opacity = 1.0;
    layer.transform.position_x = x_offset;
    layer.transform.position_y = 0.0;
    layer.transform.scale_x = 1.0;
    layer.transform.scale_y = 1.0;

    tachyon::CompositionSpec comp;
    comp.id = "main";
    comp.name = "Main";
    comp.width = 160;
    comp.height = 90;
    comp.duration = 2.0;
    comp.frame_rate = {30, 1};
    comp.layers.push_back(layer);

    tachyon::SceneSpec scene;
    scene.spec_version = "1.0";
    scene.project.id = "proj";
    scene.project.name = "Runtime";
    scene.compositions.push_back(comp);
    return scene;
}

tachyon::SceneSpec make_bezier_scene() {
    tachyon::LayerSpec layer;
    layer.id = "solid_bezier";
    layer.type = "solid";
    layer.name = "Bezier Solid";
    layer.width = 64;
    layer.height = 64;
    layer.start_time = 0.0;
    layer.in_point = 0.0;
    layer.out_point = 2.0;
    layer.fill_color.value = tachyon::ColorSpec{255, 255, 255, 255};
    layer.opacity_property.keyframes.push_back({0.0, 0.0});
    layer.opacity_property.keyframes.push_back({2.0, 1.0});
    layer.opacity_property.keyframes.front().easing = tachyon::animation::EasingPreset::Custom;
    layer.opacity_property.keyframes.front().speed_out = 0.0;
    layer.opacity_property.keyframes.front().influence_out = 90.0;
    layer.opacity_property.keyframes.back().speed_in = 0.0;
    layer.opacity_property.keyframes.back().influence_in = 80.0;

    tachyon::CompositionSpec comp;
    comp.id = "main";
    comp.name = "Bezier";
    comp.width = 64;
    comp.height = 64;
    comp.duration = 2.0;
    comp.frame_rate = {30, 1};
    comp.layers.push_back(layer);

    tachyon::SceneSpec scene;
    scene.spec_version = "1.0";
    scene.project.id = "proj";
    scene.project.name = "Bezier Runtime";
    scene.compositions.push_back(comp);
    return scene;
}

tachyon::SceneSpec make_background_component_scene() {
    tachyon::LayerSpec background_layer;
    background_layer.id = "bg_layer";
    background_layer.type = "solid";
    background_layer.name = "Background Layer";
    background_layer.width = 160;
    background_layer.height = 90;
    background_layer.start_time = 0.0;
    background_layer.in_point = 0.0;
    background_layer.out_point = 2.0;
    background_layer.fill_color.value = tachyon::ColorSpec{16, 32, 48, 255};

    tachyon::ComponentSpec background_component;
    background_component.id = "bg_component";
    background_component.name = "Background Component";
    background_component.layers.push_back(background_layer);

    tachyon::LayerSpec foreground_layer;
    foreground_layer.id = "fg_layer";
    foreground_layer.type = "solid";
    foreground_layer.name = "Foreground Layer";
    foreground_layer.width = 160;
    foreground_layer.height = 90;
    foreground_layer.start_time = 0.0;
    foreground_layer.in_point = 0.0;
    foreground_layer.out_point = 2.0;
    foreground_layer.fill_color.value = tachyon::ColorSpec{255, 255, 255, 255};

    tachyon::CompositionSpec comp;
    comp.id = "main";
    comp.name = "Main";
    comp.width = 160;
    comp.height = 90;
    comp.duration = 2.0;
    comp.frame_rate = {30, 1};
    comp.background = tachyon::BackgroundSpec::from_string("bg_component");
    comp.components.push_back(background_component);
    comp.component_instances.push_back({"bg_component", "bg_instance", {}});
    comp.layers.push_back(foreground_layer);

    tachyon::SceneSpec scene;
    scene.spec_version = "1.0";
    scene.project.id = "proj";
    scene.project.name = "Background Component Runtime";
    scene.compositions.push_back(comp);
    return scene;
}

tachyon::RenderPlan make_plan() {
    tachyon::RenderPlan plan;
    plan.job_id = "job_1";
    plan.scene_ref = "scene.json";
    plan.composition_target = "main";
    plan.composition.id = "main";
    plan.composition.name = "Main";
    plan.composition.width = 160;
    plan.composition.height = 90;
    plan.composition.duration = 2.0;
    plan.composition.frame_rate = {30, 1};
    plan.composition.layer_count = 1;
    plan.output.destination.path = "out";
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
    plan.frame_range = {0, 2};
    return plan;
}

tachyon::FrameRenderTask make_task(std::int64_t frame_number) {
    tachyon::FrameRenderTask task;
    task.frame_number = frame_number;
    task.cache_key = {"frame:key:" + std::to_string(frame_number)};
    task.cacheable = true;
    task.invalidates_when_changed = {"scene.transform.position_x"};
    return task;
}

} // namespace

bool run_frame_executor_tests() {
    using namespace tachyon;

    g_failures = 0;

    FrameCache cache;
    RenderContext render_context;
    render_context.policy = make_quality_policy("draft");
    const RenderPlan plan = make_plan();
    const SceneSpec scene = make_scene();

    SceneCompiler compiler;
    const auto compiled_result = compiler.compile(scene);
    if (!compiled_result.ok()) {
        std::cerr << "FAIL: Scene compilation failed\n";
        return false;
    }

    const ExecutedFrame first = execute_frame_task(scene, *compiled_result.value, plan, make_task(0), cache, render_context);
    const ExecutedFrame second = execute_frame_task(scene, *compiled_result.value, plan, make_task(0), cache, render_context);
    check_true(!first.cache_hit, "First execution is cache miss");
    check_true(second.cache_hit, "Second execution is cache hit");

    FrameCache isolated_cache;
    const ExecutedFrame original = execute_frame_task(scene, *compiled_result.value, plan, make_task(1), isolated_cache, render_context);
    
    const SceneSpec scene_changed = make_scene(25.0);
    const auto compiled_changed = compiler.compile(scene_changed);
    const ExecutedFrame changed = execute_frame_task(scene_changed, *compiled_changed.value, plan, make_task(1), isolated_cache, render_context);
    check_true(!original.cache_hit, "Original frame renders normally");
    check_true(!changed.cache_hit, "Changed parameter invalidates cached frame");

    const EvaluatedFrameState state = evaluate_frame_state(scene, *compiled_result.value, plan, make_task(2));
    const renderer2d::DrawList2D draw_list = build_draw_list(state);
    check_true(!draw_list.commands.empty(), "Draw list contains commands");
    if (!draw_list.commands.empty()) {
        check_true(draw_list.commands.front().kind == renderer2d::DrawCommandKind::Clear,
                   "First draw command is Clear");
    }
    check_true(draw_list.commands.size() == 2, "Single solid layer produces clear plus one layer command");
    if (draw_list.commands.size() >= 2) {
        check_true(draw_list.commands[1].kind == renderer2d::DrawCommandKind::SolidRect,
                   "Solid layer maps to SolidRect command");
    }

    const ExecutedFrame rendered = execute_frame_task(scene, *compiled_result.value, plan, make_task(2), cache, render_context);
    check_true(rendered.draw_command_count == draw_list.commands.size(),
               "Executor reports draw command count from renderer draw list");
    check_true(rendered.frame->width() == static_cast<std::uint32_t>(plan.composition.width),
               "Draft policy should still return a full-size frame after upscale");
    check_true(rendered.frame->height() == static_cast<std::uint32_t>(plan.composition.height),
               "Draft policy should still return a full-size frame after upscale");

    const SceneSpec bezier_scene = make_bezier_scene();
    const auto bezier_compiled = compiler.compile(bezier_scene);
    if (!bezier_compiled.ok()) {
        std::cerr << "FAIL: Bezier scene compilation failed\n";
        return false;
    }

    RenderPlan bezier_plan = make_plan();
    bezier_plan.composition.width = 64;
    bezier_plan.composition.height = 64;
    bezier_plan.composition.frame_rate = {30, 1};
    bezier_plan.composition_target = "main";
    bezier_plan.composition.id = "main";
    bezier_plan.composition.width = 64;
    bezier_plan.composition.height = 64;
    bezier_plan.composition.duration = 2.0;

    const ExecutedFrame bezier_frame = execute_frame_task(bezier_scene, *bezier_compiled.value, bezier_plan, make_task(30), cache, render_context);
    check_true(bezier_frame.frame != nullptr, "Bezier frame renders");
    if (bezier_frame.frame) {
        const auto pixel = bezier_frame.frame->get_pixel(32, 32);
        check_true(pixel.a > 0.0f, "Bezier opacity produces visible alpha");
    }

    const SceneSpec background_scene = make_background_component_scene();
    const auto background_compiled = compiler.compile(background_scene);
    if (!background_compiled.ok()) {
        std::cerr << "FAIL: Background component scene compilation failed\n";
        return false;
    }

    check_true(background_compiled.value->compositions.size() == 1, "Background component scene compiles one composition");
    if (background_compiled.value->compositions.size() == 1) {
        const auto& layers = background_compiled.value->compositions[0].layers;
        check_true(layers.size() == 2, "Background component expands to one background layer plus one foreground layer");
        if (layers.size() == 2) {
            check_true(layers[0].fill_color == ColorSpec{16, 32, 48, 255}, "Background component layer is ordered behind explicit layers");
            check_true(layers[1].fill_color == ColorSpec{255, 255, 255, 255}, "Foreground layer remains after background component");
        }
    }

    return g_failures == 0;
}

TEST(FrameExecutorBackgroundTest, BackgroundComponentIsExpandedBehindExplicitLayers) {
    using namespace tachyon;

    SceneCompiler compiler;
    const SceneSpec scene = make_background_component_scene();
    const auto compiled = compiler.compile(scene);
    ASSERT_TRUE(compiled.ok());
    ASSERT_EQ(compiled.value->compositions.size(), 1u);

    const auto& layers = compiled.value->compositions[0].layers;
    ASSERT_EQ(layers.size(), 2u);
    const ColorSpec expected_bg{16, 32, 48, 255};
    const ColorSpec expected_fg{255, 255, 255, 255};
    EXPECT_EQ(layers[0].fill_color, expected_bg);
    EXPECT_EQ(layers[1].fill_color, expected_fg);
}
