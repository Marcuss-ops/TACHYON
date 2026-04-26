#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "cli_internal.h"
#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>

namespace tachyon {

// Helper to load job file
bool load_job_file(const std::filesystem::path& job_path, RenderJob& job, std::ostream& err) {
    auto parsed = parse_render_job_file(job_path.string());
    if (!parsed.value.has_value()) {
        err << "Failed to parse job file: " << job_path << "\n";
        return false;
    }
    auto validation = validate_render_job(*parsed.value);
    if (!validation.ok()) {
        err << "Invalid job file: " << job_path << "\n";
        return false;
    }
    job = std::move(*parsed.value);
    return true;
}

bool run_watch_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    if (options.scene_path.empty() || options.job_path.empty()) {
        err << "--scene and --job are required for watch\n";
        return false;
    }

    out << "Starting Live Dev Mode...\n";
    
    SceneSpec scene;
    AssetResolutionTable assets;
    if (!load_scene_context(options.scene_path, scene, assets, err)) return false;

    RenderJob job;
    if (!load_job_file(options.job_path, job, err)) return false;

    // Build render plan
    auto plan_result = build_render_plan(scene, job);
    if (!plan_result.value.has_value()) {
        err << "Failed to build render plan\n";
        return false;
    }
    RenderPlan plan = std::move(*plan_result.value);

    auto exec_result = build_render_execution_plan(plan, assets.size());
    if (!exec_result.value.has_value()) {
        err << "Failed to build execution plan\n";
        return false;
    }

    SceneCompiler compiler;
    auto compiled_result = compiler.compile(scene);
    if (!compiled_result.ok()) return false;
    CompiledScene compiled = std::move(*compiled_result.value);

    RenderSession session;
    if (options.memory_budget_bytes.has_value()) {
        session.set_memory_budget_bytes(*options.memory_budget_bytes);
    }

    // Render initial frames
    std::filesystem::path output_path = job.output.destination.path.empty() ? 
        std::filesystem::path{} : std::filesystem::path(job.output.destination.path);
    out << "Initial render starting...\n";
    auto session_result = session.render(scene, compiled, *exec_result.value, output_path);
    out << "Initial render complete. Watching for changes...\n";
    
    // Track file modification times
    auto last_scene_time = std::filesystem::last_write_time(options.scene_path);
    auto last_job_time = std::filesystem::last_write_time(options.job_path);

    // Watch loop
    while (true) {
        try {
            bool changed = false;
            
            // Check scene file
            auto current_scene_time = std::filesystem::last_write_time(options.scene_path);
            if (current_scene_time > last_scene_time) {
                out << "Scene change detected. Reloading...\n";
                last_scene_time = current_scene_time;
                changed = true;
                
                if (load_scene_context(options.scene_path, scene, assets, err)) {
                    if (compiler.update_compiled_scene(compiled, scene)) {
                        out << "Scene hot-reload successful.\n";
                    }
                }
            }
            
            // Check job file
            auto current_job_time = std::filesystem::last_write_time(options.job_path);
            if (current_job_time > last_job_time) {
                out << "Job change detected. Reloading...\n";
                last_job_time = current_job_time;
                changed = true;
                
                if (load_job_file(options.job_path, job, err)) {
                    auto new_plan_result = build_render_plan(scene, job);
                    if (new_plan_result.value.has_value()) {
                        plan = std::move(*new_plan_result.value);
                        out << "Job reloaded successfully.\n";
                    }
                }
            }
            
            if (changed) {
                out << "Re-rendering affected frames...\n";
                session_result = session.render(scene, compiled, *exec_result.value, output_path);
                out << "Re-render complete. Waiting for changes...\n";
            }
        } catch (const std::exception& e) {
            err << "Watch error: " << e.what() << "\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return true;
}

} // namespace tachyon
