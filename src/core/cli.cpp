#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/core.h"
#include "cli/cli_internal.h"
#include <iostream>

namespace tachyon {

namespace {

void print_help(std::ostream& out) {
    out << "TACHYON " << version_string() << '\n';
    out << "Usage:\n";
    out << "  tachyon version\n";
    out << "  tachyon validate --scene <file> [--job <file>] [--json]\n";
    out << "  tachyon inspect --scene <file> [--job <file>] [--json]\n";
    out << "  tachyon render --cpp <scene.cpp> [--out <file>] [--workers <n>] [--quality <draft|high|production>]\n";
    out << "  tachyon render --scene <file> --job <file> [--out <file>] [--workers <n>] [--frames <start-end>]\n";
    out << "  tachyon render --batch <jobs.json> [--workers <n>] [--json]\n";
    out << "  tachyon preview --cpp <scene.cpp> [--out <file.png>] [--frame <n>]\n";
    out << "  tachyon preview --preset <id> [--out <file.png>] [--frame <n>]\n";
    out << "  tachyon studio-demo [--library <dir>] [--transition <id>] [--output-dir <dir>] [--json]\n";
}

} // namespace

int run_cli(int argc, char** argv) {
    const auto parsed = parse_cli_options(argc, argv);
    if (!parsed.value.has_value()) {
        print_diagnostics(parsed.diagnostics, std::cerr);
        return 1;
    }

    const CliOptions& options = *parsed.value;
    if (options.command == "version") {
        std::cout << version_string() << '\n';
        return 0;
    }

    if (options.command == "validate") {
        if (options.scene_path.empty()) {
            std::cerr << "--scene is required\n";
            return 1;
        }
        return run_validate_command(options, std::cout, std::cerr) ? 0 : 2;
    }

    if (options.command == "inspect") {
        if (options.scene_path.empty()) {
            std::cerr << "--scene is required\n";
            return 1;
        }
        return run_inspect_command(options, std::cout, std::cerr) ? 0 : 2;
    }

    if (options.command == "render") {
        if (options.batch_path.empty() && options.cpp_path.empty() && !options.preset_id.has_value() && (options.scene_path.empty() || options.job_path.empty())) {
            std::cerr << "Either --cpp, --preset or (--scene and --job) are required for render\n";
            return 1;
        }
        return run_render_command(options, std::cout, std::cerr) ? 0 : 2;
    }

    if (options.command == "preview-frame") {
        if (options.scene_path.empty() || options.job_path.empty() || !options.preview_frame_number.has_value() || options.preview_output.empty()) {
            std::cerr << "--scene, --job, --frame and --out are required for preview-frame\n";
            return 1;
        }
        return run_preview_frame_command(options, std::cout, std::cerr) ? 0 : 2;
    }

    if (options.command == "watch") {
        return run_watch_command(options, std::cout, std::cerr) ? 0 : 2;
    }

    if (options.command == "studio-demo") {
        return run_studio_demo_command(options, std::cout, std::cerr) ? 0 : 2;
    }

    if (options.command == "preview") {
        if (options.cpp_path.empty() && !options.preset_id.has_value() && options.scene_path.empty()) {
            std::cerr << "Either --cpp, --preset or --scene is required for preview\n";
            return 1;
        }
        return run_preview_command(options, std::cout, std::cerr) ? 0 : 2;
    }

    print_help(std::cerr);
    return 1;
}

} // namespace tachyon
