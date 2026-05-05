#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/core.h"
#include "cli/cli_internal.h"
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace tachyon {

namespace {

// ── Command registry ──────────────────────────────────────────────────────────
//
// To add a new command:
//   1. Create src/core/cli/cli_<name>.cpp with bool run_<name>_command(...)
//   2. Declare it in cli/cli_internal.h
//   3. Add one entry to kCommands below — nothing else to change.
//
struct CommandEntry {
    const char* name;
    const char* usage;
    // Returns false (+ prints to err) when required args are missing.
    std::function<bool(const CliOptions&, std::ostream&)> validate;
    std::function<bool(const CliOptions&, std::ostream&, std::ostream&)> handler;
};

static const std::vector<CommandEntry> kCommands = {
    {
        "validate",
        "tachyon validate --cpp <scene.cpp> [--job <file>]",
        [](const CliOptions& o, std::ostream& e) {
            if (o.cpp_path.empty() && !o.preset_id.has_value()) {
                e << "Either --cpp or --preset is required for validate\n";
                return false;
            }
            return true;
        },
        run_validate_command
    },
    {
        "inspect",
        "tachyon inspect --cpp <scene.cpp> [--job <file>] [--json] [--info] [--samples <n>]",
        [](const CliOptions& o, std::ostream& e) {
            if (o.cpp_path.empty() && !o.preset_id.has_value()) {
                e << "Either --cpp or --preset is required for inspect\n";
                return false;
            }
            return true;
        },
        run_inspect_command
    },
    {
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
    },
    {
        "render",
        "tachyon render --cpp <scene.cpp> --out <file> [--frames <s-e>] [--quality draft|high|production] [--workers <n>]\n"
        "        render --preset <id> --out <file>",
        [](const CliOptions& o, std::ostream& e) {
            if (o.cpp_path.empty() && !o.preset_id.has_value()) {
                e << "Either --cpp or --preset required for render\n";
                return false;
            }
            return true;
        },
        run_render_command
    },
    {
        "preview",
        "tachyon preview --cpp <scene.cpp> [--out <file.png>] [--frame <n>]\n"
        "        preview --preset <id>  [--out <file.png>] [--frame <n>]",
        [](const CliOptions& o, std::ostream& e) {
            if (o.cpp_path.empty() && !o.preset_id.has_value()) {
                e << "Either --cpp or --preset is required for preview\n";
                return false;
            }
            return true;
        },
        run_preview_command
    },
    {
        "preview-frame",
        "tachyon preview-frame --cpp <scene.cpp>  --job <file> --frame <n> --out <file.png>",
        [](const CliOptions& o, std::ostream& e) {
            if (o.cpp_path.empty()) {
                e << "--cpp is required for preview-frame\n";
                return false;
            }
            if (o.job_path.empty() || !o.preview_frame_number.has_value() || o.preview_output.empty()) {
                e << "--job, --frame and --out are required for preview-frame\n";
                return false;
            }
            return true;
        },
        run_preview_frame_command
    },
    {
        "watch",
        "tachyon watch --cpp <scene.cpp> --job <file> [--workers <n>]",
        nullptr,
        run_watch_command
    },
#ifdef TACHYON_ENABLE_CATALOG_COMMANDS
    {
        "catalog-demo",
        "tachyon catalog-demo [--catalog <dir>] [--transition <id>] [--output-dir <dir>]",
        nullptr,
        run_catalog_demo_command
    },
#endif
    {
        "fetch-fonts",
        "tachyon fetch-fonts --family <name> [--weights <w1,w2,...>] [--subsets <s1,...>] [--dest <dir>] [--overwrite]",
        nullptr,
        run_fetch_fonts_command
    },
};

void print_help(std::ostream& out) {
    out << "TACHYON " << version_string() << '\n';
    out << "Usage:\n";
    out << "  tachyon version\n";
    for (const auto& cmd : kCommands) {
        // Indent continuation lines
        std::string usage = cmd.usage;
        out << "  " << usage << '\n';
    }
}

} // namespace

int run_cli(int argc, char** argv) {
    const auto parsed = parse_cli_options(argc, argv);
    if (!parsed.value.has_value()) {
        print_diagnostics(parsed.diagnostics, std::cerr);
        return 1;
    }

    const CliOptions& options = *parsed.value;

    if (options.command == "version" || options.show_version) {
        std::cout << version_string() << '\n';
        return 0;
    }

    // Dispatch through registry
    for (const auto& cmd : kCommands) {
        if (options.command != cmd.name) continue;
        if (cmd.validate && !cmd.validate(options, std::cerr)) return 1;
        return cmd.handler(options, std::cout, std::cerr) ? 0 : 2;
    }

    print_help(std::cerr);
    return 1;
}

} // namespace tachyon
