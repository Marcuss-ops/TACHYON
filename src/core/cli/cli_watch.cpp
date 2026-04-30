#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/runtime/resource/render_context.h"
#include "preview_window.h"
#include "cli_internal.h"
#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>

namespace tachyon {

bool run_watch_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    if (options.scene_path.empty() || options.job_path.empty()) {
        err << "--scene and --job are required for watch\n";
        return false;
    }

    out << "Starting Resident Render Session with Native Preview...\n";
    
    SceneSpec scene;
    AssetResolutionTable assets;
    if (!load_scene_context(options.scene_path, scene, assets, err)) return false;

    const auto job_parsed = parse_render_job_file(options.job_path);
    if (!job_parsed.value.has_value()) {
        err << "Failed to parse job file\n";
        return false;
    }
    RenderJob job = *job_parsed.value;

    auto plan_result = build_render_plan(scene, job);
    if (!plan_result.value.has_value()) {
        err << "Failed to build render plan\n";
        return false;
    }
    RenderPlan plan = *plan_result.value;

    SceneCompiler compiler;
    auto compiled_result = compiler.compile(scene);
    if (!compiled_result.ok()) return false;
    CompiledScene compiled = std::move(*compiled_result.value);

    RenderSession session;
    if (options.memory_budget_bytes.has_value()) {
        session.set_memory_budget_bytes(*options.memory_budget_bytes);
    }
    
    // Setup Context
    RenderContext context;
    context.media = std::make_shared<media::MediaManager>();
    // Need a dummy or actual font registry depending on what's available
    
    // Initialize Window
    cli::PreviewWindow window(
        static_cast<int>(plan.composition.width), 
        static_cast<int>(plan.composition.height), 
        "Tachyon Live Preview - " + scene.project.name
    );

    auto last_time = std::filesystem::last_write_time(options.scene_path);
    std::int64_t current_frame = plan.frame_range.start;

    out << "Preview ready. Watching for changes...\n";

    while (window.poll_events()) {
        try {
            auto current_time = std::filesystem::last_write_time(options.scene_path);
            if (current_time > last_time) {
                out << "Change detected. Hot-reloading...\n";
                last_time = current_time;
                
                if (load_scene_context(options.scene_path, scene, assets, err)) {
                    if (compiler.update_compiled_scene(compiled, scene)) {
                        out << "Instant update successful.\n";
                        // Rebuild plan
                        plan_result = build_render_plan(scene, job);
                        if (plan_result.value.has_value()) {
                            plan = *plan_result.value;
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            err << "Watch error: " << e.what() << "\n";
        }

        // Render current frame
        FrameRenderTask task;
        task.frame_number = current_frame;
        task.time_seconds = static_cast<double>(current_frame) / plan.composition.frame_rate.value();

        auto executed_frame = execute_frame_task(scene, compiled, plan, task, session.cache(), context);
        if (executed_frame.frame) {
            window.present(*executed_frame.frame);
        }

        // Loop animation
        current_frame++;
        if (current_frame > plan.frame_range.end) {
            current_frame = plan.frame_range.start;
        }

        // Approx 60fps pacing for preview
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    out << "Preview window closed. Exiting watch mode.\n";
    return true;
}

} // namespace tachyon
