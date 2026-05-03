#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/report.h"
#include "tachyon/core/spec/validation/scene_validator.h"
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

    SceneSpec& scene = loaded.context->scene;
    AssetResolutionTable& assets = loaded.context->assets;

    // Run the actual SceneValidator
    core::SceneValidator validator;
    auto validation_result = validator.validate(scene);

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
        out << make_validate_report_json(scene, assets, validation_result.is_valid(), job_valid, job, validation_result) << '\n';
        return validation_result.is_valid();
    }

    // Print validation results
    if (!validation_result.is_valid()) {
        err << "Validation FAILED\n";
        for (const auto& issue : validation_result.issues) {
            const char* severity_str = "";
            switch (issue.severity) {
                case core::ValidationIssue::Severity::Fatal: severity_str = "FATAL"; break;
                case core::ValidationIssue::Severity::Error: severity_str = "ERROR"; break;
                case core::ValidationIssue::Severity::Warning: severity_str = "WARNING"; break;
            }
            err << "[" << severity_str << "] " << issue.path << ": " << issue.message << "\n";
        }
        err << "Summary: " << validation_result.fatal_count << " fatal, " 
            << validation_result.error_count << " errors, " 
            << validation_result.warning_count << " warnings\n";
        return false;
    }

    out << "scene spec valid\n";
    out << "resolved assets: " << assets.size() << '\n';
    if (job_valid) out << "render job valid\n";
    if (validation_result.warning_count > 0) {
        out << "warnings: " << validation_result.warning_count << "\n";
        for (const auto& issue : validation_result.issues) {
            if (issue.severity == core::ValidationIssue::Severity::Warning) {
                out << "[WARNING] " << issue.path << ": " << issue.message << "\n";
            }
        }
    }
    return true;
}

} // namespace tachyon
