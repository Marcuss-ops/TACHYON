#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/report.h"
#include "tachyon/runtime/runtime_facade.h"
#include "tachyon/core/cli_scene_loader.h"
#include "cli_internal.h"
#include <iostream>

namespace tachyon {

bool run_validate_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    SceneLoadOptions load_opts;
    load_opts.cpp_path = options.cpp_path;
    load_opts.preset_id = options.preset_id;

    auto loaded = load_scene_for_cli(load_opts, SceneLoadMode::Validate, out, err);
    if (!loaded.success) {
        print_diagnostics(loaded.diagnostics, err);
        return false;
    }

    auto result = RuntimeFacade::instance().validate_scene(loaded.context->scene);

    if (!result.valid) {
        err << "Validation FAILED\n";
        for (const auto& error : result.errors) {
            err << "[ERROR] " << error << "\n";
        }
        for (const auto& warn : result.warnings) {
            err << "[WARNING] " << warn << "\n";
        }
        return false;
    }

    out << "scene spec valid\n";
    if (!result.warnings.empty()) {
        out << "warnings: " << result.warnings.size() << "\n";
        for (const auto& warn : result.warnings) {
            out << "[WARNING] " << warn << "\n";
        }
    }
    return true;
}

} // namespace tachyon
