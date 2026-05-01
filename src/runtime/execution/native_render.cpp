#include "tachyon/runtime/execution/native_render.h"
#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/render_progress_sink.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace tachyon {

namespace {

RenderProgressSink* get_sink(RenderProgressSink* sink) {
    static ConsoleRenderProgressSink console_sink;
    return sink ? sink : &console_sink;
}

} // anonymous namespace

RenderSessionResult NativeRenderer::render(
    const SceneSpec& scene,
    const RenderJob& job,
    const Options& options) {
    
    RenderProgressSink* sink = get_sink(options.progress_sink);
    RenderSessionResult result;
    const auto total_start = std::chrono::high_resolution_clock::now();

    // 1. Compile the scene
    sink->on_phase_start(RenderPhase::CompileScene);
    const auto phase1_start = std::chrono::high_resolution_clock::now();
    SceneCompiler compiler;
    const auto compiled_result = compiler.compile(scene);
    const auto phase1_end = std::chrono::high_resolution_clock::now();
    double phase1_ms = std::chrono::duration<double, std::milli>(phase1_end - phase1_start).count();
    sink->on_phase_complete(RenderPhase::CompileScene, phase1_ms);
    result.scene_compile_ms = phase1_ms;

    if (!compiled_result.ok()) {
        sink->on_message("Compilation failed!");
        result.diagnostics.append(compiled_result.diagnostics);
        return result;
    }

    // 2. Build the render plan
    sink->on_phase_start(RenderPhase::BuildRenderPlan);
    const auto phase2_start = std::chrono::high_resolution_clock::now();
    const auto plan_result = build_render_plan(scene, job);
    const auto phase2_end = std::chrono::high_resolution_clock::now();
    double phase2_ms = std::chrono::duration<double, std::milli>(phase2_end - phase2_start).count();
    sink->on_phase_complete(RenderPhase::BuildRenderPlan, phase2_ms);
    result.plan_build_ms = phase2_ms;

    if (!plan_result.value.has_value()) {
        sink->on_message("Render plan build failed!");
        result.diagnostics.append(plan_result.diagnostics);
        return result;
    }

    // 3. Build the execution plan
    sink->on_phase_start(RenderPhase::BuildExecutionPlan);
    const auto phase3_start = std::chrono::high_resolution_clock::now();
    const auto execution_result = build_render_execution_plan(*plan_result.value, scene.assets.size());
    const auto phase3_end = std::chrono::high_resolution_clock::now();
    double phase3_ms = std::chrono::duration<double, std::milli>(phase3_end - phase3_start).count();
    sink->on_phase_complete(RenderPhase::BuildExecutionPlan, phase3_ms);
    result.execution_plan_build_ms = phase3_ms;

    if (!execution_result.value.has_value()) {
        sink->on_message("Execution plan build failed!");
        result.diagnostics.append(execution_result.diagnostics);
        return result;
    }

    // 4. Initialize the session and render
    sink->on_phase_start(RenderPhase::InitializeSession);
    RenderSession session;
    if (options.memory_budget_bytes.has_value()) {
        session.set_memory_budget_bytes(*options.memory_budget_bytes);
    }

    const std::size_t concurrency = (options.worker_count > 0) 
        ? options.worker_count 
        : std::thread::hardware_concurrency();

    if (options.verbose) {
        sink->on_message("Starting render: " + job.job_id);
        sink->on_message("Composition: " + job.composition_target);
        sink->on_message("Frames: " + std::to_string(job.frame_range.start) + " -> " + std::to_string(job.frame_range.end));
        sink->on_message("Output: " + job.output.destination.path);
    }

    sink->on_phase_start(RenderPhase::Render);
    result = session.render(
        scene, 
        *compiled_result.value, 
        *execution_result.value, 
        job.output.destination.path, 
        concurrency);
    sink->on_phase_complete(RenderPhase::Render);

    const auto total_end = std::chrono::high_resolution_clock::now();
    result.wall_time_total_ms = std::chrono::duration<double, std::milli>(total_end - total_start).count();

    if (result.frames_written > 0) {
        result.wall_time_per_frame_ms = result.wall_time_total_ms / result.frames_written;
    }

    return result;
}

bool NativeRenderer::render_still(
    const SceneSpec& scene,
    const std::string& composition_id,
    std::int64_t frame_number,
    const std::filesystem::path& output_path) {
    
    RenderJob job = RenderJobBuilder::still_image(composition_id, frame_number, output_path.string());
    
    const auto result = render(scene, job);
    return result.output_error.empty() && !result.frames.empty();
}

} // namespace tachyon
