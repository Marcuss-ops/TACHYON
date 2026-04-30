#include "tachyon/runtime/execution/native_render.h"
#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace tachyon {

RenderSessionResult NativeRenderer::render(
    const SceneSpec& scene,
    const RenderJob& job,
    const Options& options) {
    
    RenderSessionResult result;
    const auto total_start = std::chrono::high_resolution_clock::now();

    // 1. Compile the scene
    std::cout << "[NativeRender] Compiling scene...\n";
    const auto phase1_start = std::chrono::high_resolution_clock::now();
    SceneCompiler compiler;
    const auto compiled_result = compiler.compile(scene);
    const auto phase1_end = std::chrono::high_resolution_clock::now();
    result.scene_compile_ms = std::chrono::duration<double, std::milli>(phase1_end - phase1_start).count();

    if (!compiled_result.ok()) {
        std::cout << "[NativeRender] Compilation failed!\n";
        result.diagnostics.append(compiled_result.diagnostics);
        return result;
    }

    // 2. Build the render plan
    std::cout << "[NativeRender] Building render plan...\n";
    const auto phase2_start = std::chrono::high_resolution_clock::now();
    const auto plan_result = build_render_plan(scene, job);
    const auto phase2_end = std::chrono::high_resolution_clock::now();
    result.plan_build_ms = std::chrono::duration<double, std::milli>(phase2_end - phase2_start).count();

    if (!plan_result.value.has_value()) {
        std::cout << "[NativeRender] Render plan build failed!\n";
        result.diagnostics.append(plan_result.diagnostics);
        return result;
    }

    // 3. Build the execution plan
    std::cout << "[NativeRender] Building execution plan...\n";
    const auto phase3_start = std::chrono::high_resolution_clock::now();
    const auto execution_result = build_render_execution_plan(*plan_result.value, scene.assets.size());
    const auto phase3_end = std::chrono::high_resolution_clock::now();
    result.execution_plan_build_ms = std::chrono::duration<double, std::milli>(phase3_end - phase3_start).count();

    if (!execution_result.value.has_value()) {
        std::cout << "[NativeRender] Execution plan build failed!\n";
        result.diagnostics.append(execution_result.diagnostics);
        return result;
    }

    // 4. Initialize the session and render
    std::cout << "[NativeRender] Initializing session...\n";
    RenderSession session;
    if (options.memory_budget_bytes.has_value()) {
        session.set_memory_budget_bytes(*options.memory_budget_bytes);
    }

    const std::size_t concurrency = (options.worker_count > 0) 
        ? options.worker_count 
        : std::thread::hardware_concurrency();

    if (options.verbose) {
        std::cout << "[NativeRender] Starting render: " << job.job_id << "\n";
        std::cout << "[NativeRender] Composition: " << job.composition_target << "\n";
        std::cout << "[NativeRender] Frames: " << job.frame_range.start << " -> " << job.frame_range.end << "\n";
        std::cout << "[NativeRender] Output: " << job.output.destination.path << "\n";
    }

    result = session.render(
        scene, 
        *compiled_result.value, 
        *execution_result.value, 
        job.output.destination.path, 
        concurrency);

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
    
    RenderJob job;
    job.job_id = "still_render_" + composition_id;
    job.composition_target = composition_id;
    job.frame_range = {frame_number, frame_number};
    job.output.destination.path = output_path.string();
    job.output.destination.overwrite = true;
    
    // Default to high quality PNG sequence (one frame)
    job.output.profile.name = "png-sequence";
    job.output.profile.container = "png";
    job.output.profile.video.codec = "png";
    job.output.profile.video.pixel_format = "rgba8";
    
    const auto result = render(scene, job);
    return result.output_error.empty() && !result.frames.empty();
}

} // namespace tachyon
