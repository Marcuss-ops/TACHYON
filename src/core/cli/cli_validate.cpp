#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/report.h"
#include "cli_internal.h"
#include <iostream>

namespace tachyon {

bool run_validate_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    SceneSpec scene;
    AssetResolutionTable assets;
    if (!load_scene_context(options.scene_path, scene, assets, err)) return false;

    std::optional<RenderJob> job;
    bool job_valid = false;
    if (!options.job_path.empty()) {
        const auto parsed = parse_render_job_file(options.job_path);
        if (!parsed.value.has_value()) {
            print_diagnostics(parsed.diagnostics, err);
            return false;
        }
        job = *parsed.value;
        job_valid = true;
    }

    if (options.json_output) {
        out << make_validate_report_json(scene, assets, true, job_valid, job) << '\n';
        return true;
    }

    out << "scene spec valid\n";
    out << "resolved assets: " << assets.size() << '\n';
    if (job_valid) out << "render job valid\n";
    return true;
}

} // namespace tachyon
