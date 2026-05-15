#include "tachyon/core/cli_options.h"
#include "parsing/parse_helpers.h"
#include "parsing/parse_render.h"
#include "parsing/parse_inspect.h"
#include "parsing/parse_metrics.h"
#include "parsing/parse_tool.h"
#include <exception>
#include <string>
#include <vector>

namespace tachyon {

namespace {

std::optional<std::size_t> parse_memory_budget_mb(const std::string& value, DiagnosticBag& diagnostics) {
    try {
        return static_cast<std::size_t>(std::stoull(value));
    } catch (const std::exception&) {
        diagnostics.add_error("cli.memory_budget_invalid", "invalid value for --memory-budget-mb: " + value);
        return std::nullopt;
    }
}

} // namespace

ParseResult<CliOptions> parse_cli_options(int argc, char** argv) {
    ParseResult<CliOptions> result;
    const std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty()) {
        result.diagnostics.add_error("cli.command_missing", "a command is required");
        return result;
    }

    CliOptions options;
    if (args[0] == "--version" || args[0] == "-v") {
        options.show_version = true;
        options.command = "version";
        result.value = std::move(options);
        return result;
    }
    options.command = args[0];

    for (std::size_t index = 1; index < args.size(); ++index) {
        const std::string& arg = args[index];

        if (arg == "--scene") {
            result.diagnostics.add_error("cli.scene_deprecated", "--scene loading is no longer supported. Use --cpp for C++ scenes or --preset.");
            return result;
        }

        if (parse_render_option(arg, args, index, options, result.diagnostics)) continue;
        if (parse_inspect_option(arg, args, index, options, result.diagnostics)) continue;
        if (parse_metrics_option(arg, args, index, options, result.diagnostics)) continue;
        if (parse_tool_option(arg, args, index, options, result.diagnostics)) continue;

        // Global overrides
        if (arg == "--library") {
            options.library_path = require_argument(args, index);
            if (options.library_path.empty()) result.diagnostics.add_error("cli.library_missing", "missing value for --library");
            continue;
        }
        if (arg == "--memory-budget-mb") {
            std::string val = require_argument(args, index);
            if (val.empty()) result.diagnostics.add_error("cli.memory_budget_missing", "missing value for --memory-budget-mb");
            else {
                auto parsed = parse_memory_budget_mb(val, result.diagnostics);
                if (parsed) options.memory_budget_bytes = (*parsed) * 1024ULL * 1024ULL;
            }
            continue;
        }
        if (arg == "--output-dir") {
            options.output_dir = require_argument(args, index);
            if (options.output_dir.empty()) result.diagnostics.add_error("cli.output_dir_missing", "missing value for --output-dir");
            continue;
        }
        if (arg == "--format") {
            options.output_format = require_argument(args, index);
            if (options.output_format.empty()) result.diagnostics.add_error("cli.format_missing", "missing value for --format");
            continue;
        }
        if (arg == "--transition") {
            options.transition_id = require_argument(args, index);
            if (options.transition_id->empty()) result.diagnostics.add_error("cli.transition_missing", "missing value for --transition");
            continue;
        }
        if (arg == "--output-preset") {
            options.render.output_preset_id = require_argument(args, index);
            if (options.render.output_preset_id->empty()) result.diagnostics.add_error("cli.output_preset_missing", "missing value for --output-preset");
            continue;
        }
        
        // Output-presets subcommands
        if (index == 1 && options.command == "output-presets") {
            if (arg == "list" || arg == "info") { options.output_presets.command = arg; continue; }
            if (!arg.empty() && arg.front() != '-') {
                result.diagnostics.add_error("cli.output_presets_subcommand_invalid", "unknown output-presets subcommand: " + arg);
                return result;
            }
        }
        if (options.command == "output-presets" && options.output_presets.command == "info" && index == 2 && options.output_presets.name.empty()) {
            options.output_presets.name = arg;
            continue;
        }

        if (arg == "--version" || arg == "-v") { options.show_version = true; continue; }
        if (arg == "--all") { options.render.render_all_compositions = true; continue; }
        if (arg == "--cost-per-hour") {
            std::string val = require_argument(args, index);
            if (val.empty()) result.diagnostics.add_error("cli.cost_missing", "missing value for --cost-per-hour");
            else {
                try { options.metrics.machine_cost_per_hour = std::stod(val); }
                catch (...) { result.diagnostics.add_error("cli.cost_invalid", "invalid value for --cost-per-hour: " + val); }
            }
            continue;
        }

        result.diagnostics.add_error("cli.flag_unknown", "unknown flag: " + arg, arg);
        return result;
    }

    if (result.diagnostics.ok()) result.value = std::move(options);
    return result;
}

} // namespace tachyon
