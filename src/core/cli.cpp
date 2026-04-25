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
    out << "  tachyon render --scene <file> --job <file> [--out <file>] [--workers <n>] [--memory-budget-mb <n>] [--frames <start-end>] [--json]\n";
    out << "  tachyon render --batch <jobs.json> [--workers <n>] [--json]\n";
    out << "  tachyon preview-frame --scene <file> --job <file> --frame <n> --out <file.png>\n";
    out << "  tachyon watch --scene <file> --job <file> [--workers <n>]\n";
    out << "  tachyon studio-demo [--library <dir>] [--transition <id>] [--output-dir <dir>] [--json]\n";
    out << "  tachyon transition --from <file> --to <file> --out <file> [--transition <id> | --random] [--progress <0-1>]\n";
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
        if (options.batch_path.empty() && (options.scene_path.empty() || options.job_path.empty())) {
            std::cerr << "--scene and --job are required\n";
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

    if (options.command == "transition") {
        if (options.from_image.empty() || options.to_image.empty() || options.output_image.empty()) {
            std::cerr << "--from, --to and --out are required for transition command\n";
            return 1;
        }
        return run_transition_command(options, std::cout, std::cerr) ? 0 : 2;
    }

    print_help(std::cerr);
    return 1;
}

} // namespace tachyon
