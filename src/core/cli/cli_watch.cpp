#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/spec/cpp_scene_loader.h"
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
    if (options.job_path.empty()) {
        err << "--job is required for watch\n";
        return false;
    }

    bool use_cpp = !options.cpp_path.empty();
    bool use_scene = !options.scene_path.empty();

    if (!use_cpp && !use_scene) {
        err << "Either --cpp or --scene is required for watch\n";
        return false;
    }

    std::filesystem::path watch_path;
    if (use_cpp) {
        watch_path = options.cpp_path;
        out << "Starting Resident Render Session with Native Preview (C++ mode)...\n";
    } else {
        watch_path = options.scene_path;
        err << "WARNING: Watching JSON scene files is DEPRECATED. Use --cpp instead.\n";
        out << "Starting Resident Render Session with Native Preview (legacy JSON mode)...\n";
    }

    const auto job_parsed = parse_render_job_file(options.job_path);
    if (!job_parsed.value.has_value()) {
        err << "Failed to parse job file\n";
        return false;
    }
    RenderJob job = *job_parsed.value;

    auto load_watch_scene = [&](SceneSpec& sc) -> bool {
        AssetResolutionTable dummy;
        if (use_cpp) {
            auto result = CppSceneLoader::load_from_file(options.cpp_path);
            if (!result.success) {
                err << "C++ Scene Loader failed:\n" << result.diagnostics << "\n";
                return false;
            }
            sc = std::move(*result.scene);
        } else {
            return load_scene_context(options.scene_path, sc, dummy, err);
        }
        return true;
    };

    SceneSpec scene;
    if (!load_watch_scene(scene)) return false;

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

    AssetResolutionTable assets;

    RenderSession session;
    if (options.memory_budget_bytes.has_value()) {
        session.set_memory_budget_bytes(*options.memory_budget_bytes);
    }
    
    RenderContext context;
    context.media = std::make_shared<media::MediaManager>();
    
    cli::PreviewWindow window(
        static_cast<int>(plan.composition.width), 
        static_cast<int>(plan.composition.height), 
        "Tachyon Live Preview - " + scene.project.name
    );

    auto last_time = std::filesystem::last_write_time(watch_path);
    std::int64_t current_frame = plan.frame_range.start;

    out << "Preview ready. Watching for changes...\n";

    while (window.poll_events()) {
        try {
            auto current_time = std::filesystem::last_write_time(watch_path);
            if (current_time > last_time) {
                out << "Change detected. Hot-reloading...\n";
                last_time = current_time;
                
                if (load_watch_scene(scene)) {
                    if (compiler.update_compiled_scene(compiled, scene)) {
                        out << "Instant update successful.\n";
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

        FrameRenderTask task;
        task.frame_number = current_frame;
        task.time_seconds = static_cast<double>(current_frame) / plan.composition.frame_rate.value();

        auto executed_frame = execute_frame_task(scene, compiled, plan, task, session.cache(), context);
        if (executed_frame.frame) {
            window.present(*executed_frame.frame);
        }

        current_frame++;
        if (current_frame > plan.frame_range.end) {
            current_frame = plan.frame_range.start;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    out << "Preview window closed. Exiting watch mode.\n";
    return true;
}

} // namespace tachyon
