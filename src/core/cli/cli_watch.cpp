#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/runtime/compiler/scene_compiler.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/runtime/resource/render_context.h"
#include "cli_internal.h"
#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>

namespace tachyon {

bool run_watch_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& /*registry*/) {
    if (options.cpp_path.empty()) {
        err << "--cpp is required for watch\n";
        return false;
    }

    std::filesystem::path watch_path = options.cpp_path;
    out << "Starting Resident Render Session with Native Preview (C++ mode)...\n";

    auto load_watch_scene = [&](SceneSpec& sc) -> bool {
        SceneLoadOptions opts;
        opts.cpp_path = options.cpp_path;
        auto r = load_scene_for_cli(opts, SceneLoadMode::Watch, out, err);
        if (!r.success) return false;
        sc = std::move(r.context->scene);
        return true;
    };

    SceneSpec scene;
    if (!load_watch_scene(scene)) return false;

    if (scene.compositions.empty()) {
        err << "Scene has no compositions.\n";
        return false;
    }
    const auto& comp = scene.compositions.front();
    RenderJob job = RenderJobBuilder::video_export(comp.id, {0, static_cast<std::int64_t>(comp.duration * comp.frame_rate.value())}, "preview.mp4");


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
    
    auto last_time = std::filesystem::last_write_time(watch_path);
    std::int64_t current_frame = plan.frame_range.start;

    out << "Preview ready. Watching for changes...\n";
    
    while (true) {
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

        current_frame++;
        if (current_frame > plan.frame_range.end) {
            current_frame = plan.frame_range.start;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    out << "Watch mode terminated.\n";
    return true;
}

} // namespace tachyon
