#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/report.h"
#include "cli_internal.h"
#include "tachyon/text/fonts/font_manifest.h"
#include "tachyon/text/fonts/font_coverage_reporter.h"
#include <iostream>

namespace tachyon {
 
bool run_inspect_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    if (options.command == "inspect-fonts") {
        return run_inspect_fonts_command(options, out, err);
    }
 
    SceneSpec scene;
    AssetResolutionTable assets;
    if (!load_scene_context(options.scene_path, scene, assets, err)) return false;
 
    std::optional<RenderPlan> render_plan;
    std::optional<RenderExecutionPlan> execution_plan;
    if (!options.job_path.empty()) {
        const auto job_parsed = parse_render_job_file(options.job_path);
        if (!job_parsed.value.has_value()) {
            print_diagnostics(job_parsed.diagnostics, err);
            return false;
        }
 
        const auto plan_result = build_render_plan(scene, *job_parsed.value);
        if (!plan_result.value.has_value()) {
            print_diagnostics(plan_result.diagnostics, err);
            return false;
        }
        render_plan = *plan_result.value;
 
        const auto execution_result = build_render_execution_plan(*render_plan, assets.size());
        if (!execution_result.value.has_value()) {
            print_diagnostics(execution_result.diagnostics, err);
            return false;
        }
        execution_plan = *execution_result.value;
    }
 
    if (options.json_output) {
        out << make_inspect_report_json(scene, assets, render_plan, execution_plan) << '\n';
        return true;
    }
 
    print_inspect_report_text(scene, assets, render_plan, execution_plan, out);
    return true;
}

bool run_inspect_fonts_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    std::filesystem::path manifest_path = options.font_manifest_path;
    if (manifest_path.empty() && !options.scene_path.empty()) {
        manifest_path = options.scene_path;
    }
 
    auto manifest = FontManifestParser::parse_file(manifest_path);
    if (!manifest) {
        err << "Failed to parse font manifest: " << manifest_path.string() << '\n';
        return false;
    }
 
    FontCoverageReporter reporter(*manifest);
    auto summary = reporter.generate_report();
 
    if (options.json_output) {
        out << "{\n  \"fonts\": [\n";
        for (std::size_t i = 0; i < summary.font_reports.size(); ++i) {
            const auto& report = summary.font_reports[i];
            out << "    {\n";
            out << "      \"family\": \"" << report.font_family << "\",\n";
            out << "      \"path\": \"" << report.font_path.string() << "\",\n";
            out << "      \"loaded\": " << (report.loaded ? "true" : "false") << ",\n";
            out << "      \"total_glyphs\": " << report.total_glyphs << ",\n";
            out << "      \"missing_codepoints\": " << report.missing_codepoints.size() << ",\n";
            out << "      \"has_all_required\": " << (report.has_all_required ? "true" : "false") << "\n";
            out << "    }";
            if (i < summary.font_reports.size() - 1) out << ",";
            out << "\n";
        }
        out << "  ],\n";
        out << "  \"total_fonts\": " << summary.total_fonts << ",\n";
        out << "  \"total_missing_glyphs\": " << summary.total_missing_glyphs << "\n";
        out << "}\n";
    } else {
        out << "Font Coverage Report\n";
        out << "==================\n\n";
        out << "Total fonts: " << summary.total_fonts << "\n";
        out << "Total missing glyphs: " << summary.total_missing_glyphs << "\n\n";
 
        for (const auto& report : summary.font_reports) {
            out << "Font: " << report.font_family << "\n";
            out << "  Path: " << report.font_path.string() << "\n";
            out << "  Loaded: " << (report.loaded ? "Yes" : "No") << "\n";
            out << "  Total glyphs: " << report.total_glyphs << "\n";
            out << "  Missing codepoints: " << report.missing_codepoints.size() << "\n";
            if (!report.missing_codepoints.empty()) {
                out << "  Missing: ";
                for (auto cp : report.missing_codepoints) {
                    out << "U+" << std::hex << cp << std::dec << " ";
                }
                out << "\n";
            }
            out << "\n";
        }
    }
    return true;
}

} // namespace tachyon
