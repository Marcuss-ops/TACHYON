#include "tachyon/runtime/execution/frame_executor.h"
#include "tachyon/core/spec/scene_compiler.h"
#include "tachyon/runtime/core/compiled_scene.h"

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
    check_true(rendered.frame.width() == static_cast<std::uint32_t>(plan.composition.width),
               "Draft policy should still return a full-size frame after upscale");
    check_true(rendered.frame.height() == static_cast<std::uint32_t>(plan.composition.height),
               "Draft policy should still return a full-size frame after upscale");

    return g_failures == 0;
}
