#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/core/analysis/motion_map.h"
#include "cli_internal.h"

#include <algorithm>

namespace tachyon {

bool run_motion_map_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    SceneLoadOptions load_opts;
    load_opts.cpp_path = options.cpp_path;
    load_opts.preset_id = options.preset_id;

    auto loaded = load_scene_for_cli(load_opts, SceneLoadMode::Inspect, out, err);
    if (!loaded.success) {
        print_diagnostics(loaded.diagnostics, err);
        return false;
    }

    analysis::MotionMapOptions motion_options;
    motion_options.runtime_samples = options.samples_explicitly_set
        ? std::max(0, options.inspect_samples)
        : 0;

    const auto report = analysis::build_motion_map(loaded.context->scene, motion_options);
    if (options.json_output) {
        analysis::print_motion_map_json(report, out);
    } else {
        analysis::print_motion_map_text(report, out);
    }
    return true;
}

} // namespace tachyon
