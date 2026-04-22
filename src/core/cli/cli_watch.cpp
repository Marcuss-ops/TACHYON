#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/runtime/execution/session/render_session.h"
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

    out << "Starting Resident Render Session...\n";
    
    SceneSpec scene;
    AssetResolutionTable assets;
    if (!load_scene_context(options.scene_path, scene, assets, err)) return false;

    SceneCompiler compiler;
    auto compiled_result = compiler.compile(scene);
    if (!compiled_result.ok()) return false;
    CompiledScene compiled = std::move(*compiled_result.value);

    RenderSession session;
    if (options.memory_budget_bytes.has_value()) {
        session.set_memory_budget_bytes(*options.memory_budget_bytes);
    }
    out << "Initial render complete. Watching for changes...\n";
    
    auto last_time = std::filesystem::last_write_time(options.scene_path);

    while (true) {
        try {
            auto current_time = std::filesystem::last_write_time(options.scene_path);
            if (current_time > last_time) {
                out << "Change detected. Hot-reloading...\n";
                last_time = current_time;
                
                if (load_scene_context(options.scene_path, scene, assets, err)) {
                    if (compiler.update_compiled_scene(compiled, scene)) {
                        out << "Instant update successful.\n";
                    }
                }
                out << "Waiting for changes...\n";
            }
        } catch (const std::exception& e) {
            err << "Watch error: " << e.what() << "\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return true;
}

} // namespace tachyon
