#include "tachyon/runtime/execution/render_session.h"
#include "tachyon/core/spec/scene_compiler.h"
#include "tachyon/runtime/core/compiled_scene.h"

#include <filesystem>
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

tachyon::SceneSpec make_scene() {
    tachyon::LayerSpec layer;
    layer.id = "solid_1";
    layer.type = "solid";
    layer.name = "Solid One";
    layer.start_time = 0.0;
    layer.in_point = 0.0;
    layer.out_point = 2.0;
    layer.opacity = 1.0;
    layer.transform.position_x = 0.0;
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

tachyon::RenderExecutionPlan make_execution_plan() {
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
    plan.output.destination.path = "tests/output/runtime_seq";
    plan.output.destination.overwrite = true;
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

    tachyon::RenderExecutionPlan exec;
    exec.render_plan = plan;
    for (int frame = 0; frame < 3; ++frame) {
        tachyon::FrameRenderTask task;
        task.frame_number = frame;
        task.cache_key = {"frame:key:" + std::to_string(frame)};
        task.cacheable = true;
        task.invalidates_when_changed = {"scene.transform.position_x"};
        exec.frame_tasks.push_back(task);
    }
    return exec;
}

tachyon::SceneSpec make_precomp_scene() {
    tachyon::LayerSpec nested_layer;
    nested_layer.id = "nested_solid";
    nested_layer.type = "solid";
    nested_layer.name = "Nested Solid";
    nested_layer.start_time = 0.0;
    nested_layer.in_point = 0.0;
    nested_layer.out_point = 1.0;
    nested_layer.opacity = 1.0;

    tachyon::CompositionSpec nested_comp;
    nested_comp.id = "nested";
    nested_comp.name = "Nested";
    nested_comp.width = 64;
    nested_comp.height = 64;
    nested_comp.duration = 1.0;
    nested_comp.frame_rate = {24, 1};
    nested_comp.layers.push_back(nested_layer);

    tachyon::LayerSpec precomp_layer;
    precomp_layer.id = "precomp_1";
    precomp_layer.type = "precomp";
    precomp_layer.name = "Precomp One";
    precomp_layer.start_time = 0.0;
    precomp_layer.in_point = 0.0;
    precomp_layer.out_point = 1.0;
    precomp_layer.opacity = 1.0;
    precomp_layer.precomp_id = "nested";

    tachyon::CompositionSpec main_comp;
    main_comp.id = "main";
    main_comp.name = "Main";
    main_comp.width = 64;
    main_comp.height = 64;
    main_comp.duration = 1.0;
    main_comp.frame_rate = {24, 1};
    main_comp.layers.push_back(precomp_layer);

    tachyon::SceneSpec scene;
    scene.spec_version = "1.0";
    scene.project.id = "proj";
    scene.project.name = "Runtime";
    scene.compositions.push_back(main_comp);
    scene.compositions.push_back(nested_comp);
    return scene;
}

tachyon::RenderExecutionPlan make_precomp_execution_plan() {
    tachyon::RenderPlan plan;
    plan.job_id = "job_precomp";
    plan.scene_ref = "scene.json";
    plan.composition_target = "main";
    plan.composition.id = "main";
    plan.composition.name = "Main";
    plan.composition.width = 64;
    plan.composition.height = 64;
    plan.composition.duration = 1.0;
    plan.composition.frame_rate = {24, 1};
    plan.composition.layer_count = 1;
    plan.output.destination.path = "tests/output/runtime_precomp";
    plan.output.destination.overwrite = true;
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

    tachyon::RenderExecutionPlan exec;
    exec.render_plan = plan;
    tachyon::FrameRenderTask task;
    task.frame_number = 0;
    task.cache_key = {"frame:key:precomp"};
    task.cacheable = false;
    exec.frame_tasks.push_back(task);
    return exec;
}

} // namespace

bool run_render_session_tests() {
    using namespace tachyon;

    g_failures = 0;

    RenderSession session;
    const SceneSpec scene = make_scene();
    const RenderExecutionPlan execution_plan = make_execution_plan();

    SceneCompiler compiler;
    const auto compiled_result = compiler.compile(scene);
    if (!compiled_result.ok()) {
        std::cerr << "FAIL: Scene compilation failed\n";
        return false;
    }

    std::filesystem::remove_all("tests/output/runtime_seq");
    std::filesystem::create_directories("tests/output/runtime_seq");
    const RenderSessionResult first = session.render(scene, *compiled_result.value, execution_plan, "tests/output/runtime_seq");
    check_true(first.frames.size() == 3, "Render session produces three frames");
    check_true(first.cache_misses == 3, "First pass is all cache misses");
    check_true(first.frames_written == 3, "First pass writes three output frames");
    check_true(first.output_error.empty(), "First pass completes without output error");
    check_true(first.frames[0].frame->width() == 160, "Frame width matches composition");
    check_true(first.frames[0].frame->height() == 90, "Frame height matches composition");
    check_true(std::filesystem::exists("tests/output/runtime_seq/frame_000001.png"), "First PNG output exists");
    check_true(std::filesystem::exists("tests/output/runtime_seq/frame_000003.png"), "Third PNG output exists");

    const RenderSessionResult second = session.render(scene, *compiled_result.value, execution_plan, "tests/output/runtime_seq", 2);
    check_true(second.cache_hits == 3, "Second pass reuses cache for all frames");
    check_true(second.frames_written == 3, "Second pass also writes three output frames");
    check_true(second.output_error.empty(), "Second pass completes without output error");

    std::filesystem::remove_all("tests/output/runtime_precomp");
    std::filesystem::create_directories("tests/output/runtime_precomp");
    const SceneSpec precomp_scene = make_precomp_scene();
    const RenderExecutionPlan precomp_plan = make_precomp_execution_plan();
    
    const auto compiled_precomp = compiler.compile(precomp_scene);
    if (!compiled_precomp.ok()) {
        std::cerr << "FAIL: Precomp scene compilation failed\n";
        return false;
    }

    const RenderSessionResult precomp_first = session.render(precomp_scene, *compiled_precomp.value, precomp_plan, "tests/output/runtime_precomp");
    const std::size_t precomp_entries_after_first = session.precomp_cache()->entry_count();
    check_true(precomp_first.output_error.empty(), "Precomp pass completes without output error");
    check_true(precomp_entries_after_first > 0, "Precomp cache should store at least one entry");

    const RenderSessionResult precomp_second = session.render(precomp_scene, *compiled_precomp.value, precomp_plan, "tests/output/runtime_precomp");
    const std::size_t precomp_entries_after_second = session.precomp_cache()->entry_count();
    check_true(precomp_second.output_error.empty(), "Second precomp pass completes without output error");
    check_true(precomp_entries_after_second == precomp_entries_after_first, "Precomp cache should persist across render calls");

    return g_failures == 0;
}
