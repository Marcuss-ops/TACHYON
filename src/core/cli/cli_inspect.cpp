#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/report.h"
#include "tachyon/core/cli_scene_loader.h"
#include "cli_internal.h"
#include "tachyon/text/fonts/management/font_manifest.h"
#include "tachyon/text/fonts/utils/font_coverage_reporter.h"
#include <iostream>

namespace tachyon {
 
bool run_inspect_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    if (options.command == "inspect-fonts") {
        return run_inspect_fonts_command(options, out, err);
    }
 
    SceneLoadOptions load_opts;
    load_opts.cpp_path = options.cpp_path;
    load_opts.preset_id = options.preset_id;

    auto loaded = load_scene_for_cli(load_opts, SceneLoadMode::Inspect, out, err);
    if (!loaded.success) {
        print_diagnostics(loaded.diagnostics, err);
        return false;
    }

    SceneSpec& scene = loaded.context->scene;
    AssetResolutionTable& assets = loaded.context->assets;
 
    std::optional<RenderPlan> render_plan;
    std::optional<RenderExecutionPlan> execution_plan;
 
    print_inspect_report_text(scene, assets, render_plan, execution_plan, out);
    return true;
}

bool run_inspect_fonts_command(const CliOptions& /*options*/, std::ostream& /*out*/, std::ostream& err) {
    err << "Font manifest inspection is no longer supported. Please use the C++ Font API.\n";
    return false;
}

} // namespace tachyon
