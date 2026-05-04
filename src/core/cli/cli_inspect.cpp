#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/report.h"
#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/core/analysis/scene_inspector.h"
#include "cli_internal.h"
#include "tachyon/text/fonts/management/font_manifest.h"
#include "tachyon/text/fonts/utils/font_coverage_reporter.h"
#include <iostream>
#include <iomanip>
#include <string_view>

namespace tachyon {

namespace {

std::string escape_json(std::string_view value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (const char c : value) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

std::string severity_to_string(analysis::InspectionSeverity severity) {
    switch (severity) {
        case analysis::InspectionSeverity::Info: return "info";
        case analysis::InspectionSeverity::Warning: return "warning";
        case analysis::InspectionSeverity::Error: return "error";
    }
    return "info";
}

void print_inspect_json(
    const SceneSpec& scene,
    const AssetResolutionTable& assets,
    const analysis::InspectionReport& inspection,
    std::ostream& out) {
    out << "{\n";
    out << "  \"project\": {\n";
    out << "    \"id\": \"" << escape_json(scene.project.id) << "\",\n";
    out << "    \"name\": \"" << escape_json(scene.project.name) << "\",\n";
    out << "    \"authoring_tool\": \"" << escape_json(scene.project.authoring_tool) << "\"\n";
    out << "  },\n";
    out << "  \"schema_version\": \"" << escape_json(scene.schema_version.to_string()) << "\",\n";
    out << "  \"assets\": " << assets.size() << ",\n";
    out << "  \"compositions\": " << scene.compositions.size() << ",\n";
    out << "  \"ok\": " << (inspection.ok() ? "true" : "false") << ",\n";
    out << "  \"issues\": [\n";
    for (std::size_t i = 0; i < inspection.issues.size(); ++i) {
        const auto& issue = inspection.issues[i];
        out << "    {\n";
        out << "      \"severity\": \"" << severity_to_string(issue.severity) << "\",\n";
        out << "      \"code\": \"" << escape_json(issue.code) << "\",\n";
        out << "      \"path\": \"" << escape_json(issue.path) << "\",\n";
        out << "      \"message\": \"" << escape_json(issue.message) << "\",\n";
        out << "      \"time\": " << issue.time << "\n";
        out << "    }" << (i + 1 < inspection.issues.size() ? "," : "") << "\n";
    }
    out << "  ]\n";
    out << "}\n";
}

} // namespace
 
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
 
    analysis::InspectionOptions inspect_options;
    inspect_options.samples = options.inspect_samples;
    const auto inspection = analysis::inspect_scene(scene, inspect_options);

    if (options.json_output) {
        print_inspect_json(scene, assets, inspection, out);
    } else {
        print_inspect_report_text(scene, assets, render_plan, execution_plan, out);
        analysis::print_inspection_report_text(inspection, out);
    }

    return inspection.ok();
}

bool run_inspect_fonts_command(const CliOptions& /*options*/, std::ostream& /*out*/, std::ostream& err) {
    err << "Font manifest inspection is no longer supported. Please use the C++ Font API.\n";
    return false;
}

} // namespace tachyon
