#include "tachyon/runtime/registry/engine_registry.h"
#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/core/analysis/motion_map.h"
#include "cli_internal.h"
#include "command_registry.h"
#include <algorithm>

namespace tachyon {
bool run_motion_map_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& /*bundle*/) {
    SceneLoadOptions load_opts;
    load_opts.cpp_path = options.cpp_path;
    load_opts.preset_id = options.preset_id;

    auto loaded = load_scene_for_cli(load_opts, SceneLoadMode::Inspect, out, err);
    if (!loaded.success) {
        print_diagnostics(loaded.diagnostics, err);
        return false;
    }

    analysis::MotionMapOptions motion_options;
    motion_options.runtime_samples = options.inspect.samples_explicitly_set
        ? std::max(0, options.inspect.samples)
        : 0;

    const auto report = analysis::build_motion_map(loaded.context->scene, motion_options);
    if (options.inspect.json_output) {
        analysis::print_motion_map_json(report, out);
    } else {
        analysis::print_motion_map_text(report, out);
    }
    return true;
}

CommandDescriptor make_motion_map_command() {
    return {
        "motion-map",
        "tachyon motion-map --cpp <scene.cpp> [--preset <id>] [--json] [--samples <n>]",
        [](const CliOptions& o, std::ostream& e) {
            if (o.cpp_path.empty() && !o.preset_id.has_value()) {
                e << "Either --cpp or --preset is required for motion-map\n";
                return false;
            }
            return true;
        },
        run_motion_map_command
    };
}

} // namespace tachyon
