#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/core/assets/asset_resolution.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/runtime/registry/engine_registry.h"
#include "cli/commands/command.h"

#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>

namespace tachyon {

int run_watch(const CliOptions& options, std::ostream& out, std::ostream& err) {
    out << "watching scene: " << options.cpp_path << "\n";
    out << "press Ctrl+C to stop\n";

    std::filesystem::file_time_type last_write_time;
    bool first_run = true;

    while (true) {
        try {
            if (!std::filesystem::exists(options.cpp_path)) {
                err << "error: scene path does not exist: " << options.cpp_path << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(2));
                continue;
            }

            auto current_write_time = std::filesystem::last_write_time(options.cpp_path);
            if (first_run || current_write_time != last_write_time) {
                last_write_time = current_write_time;
                first_run = false;

                out << "\n--- change detected, reloading ---\n";

                SceneLoadOptions load_opts;
                load_opts.cpp_path = options.cpp_path;
                load_opts.preset_id = options.preset_id;

                auto loaded = load_scene_for_cli(load_opts, SceneLoadMode::Watch, out, err);
                if (loaded.success) {
                    out << "scene loaded successfully.\n";
                    out << "assets resolved: " << loaded.context->assets.size() << "\n";
                } else {
                    err << "failed to load scene.\n";
                    for (const auto& d : loaded.diagnostics.diagnostics) {
                        err << "  [" << diagnostic_severity_string(d.severity) << "] " << d.message << "\n";
                    }
                }
            }
        } catch (const std::exception& e) {
            err << "error during watch: " << e.what() << "\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}

bool run_watch_command(const CliOptions& options, std::ostream& out, std::ostream& err, ::tachyon::runtime::EngineRegistry& /*bundle*/) {
    return run_watch(options, out, err) == 0;
}

::tachyon::CommandDescriptor make_watch_command() {
    return {
        "watch",
        "tachyon watch --cpp <path> [--preset <id>]",
        nullptr,
        run_watch_command
    };
}

} // namespace tachyon
