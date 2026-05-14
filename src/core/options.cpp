#include "tachyon/core/cli_options.h"

#include <exception>
#include <string>
#include <vector>

namespace tachyon {
namespace {

std::string require_argument(const std::vector<std::string>& args, std::size_t& index) {
    if (index + 1 >= args.size()) {
        return {};
    }
    ++index;
    return args[index];
}

std::size_t parse_worker_count(const std::string& value, DiagnosticBag& diagnostics) {
    try {
        const std::size_t parsed = static_cast<std::size_t>(std::stoull(value));
        if (parsed == 0) {
            diagnostics.add_error("cli.workers_invalid", "--workers must be greater than zero");
            return 0;
        }
        return parsed;
    } catch (const std::exception&) {
        diagnostics.add_error("cli.workers_invalid", "invalid value for --workers: " + value);
        return 0;
    }
}

std::optional<std::size_t> parse_memory_budget_mb(const std::string& value, DiagnosticBag& diagnostics) {
    try {
        return static_cast<std::size_t>(std::stoull(value));
    } catch (const std::exception&) {
        diagnostics.add_error("cli.memory_budget_invalid", "invalid value for --memory-budget-mb: " + value);
        return std::nullopt;
    }
}

std::optional<FrameRange> parse_frame_range(const std::string& value, DiagnosticBag& diagnostics) {
    try {
        const std::size_t dash_pos = value.find('-');
        if (dash_pos == std::string::npos) {
            const std::int64_t frame = std::stoll(value);
            return FrameRange{frame, frame};
        }

        const std::int64_t start = std::stoll(value.substr(0, dash_pos));
        const std::int64_t end = std::stoll(value.substr(dash_pos + 1));
        
        if (start > end) {
            diagnostics.add_error("cli.frames_invalid", "frame start must be less than or equal to end: " + value);
            return std::nullopt;
        }

        return FrameRange{start, end};
    } catch (const std::exception&) {
        diagnostics.add_error("cli.frames_invalid", "invalid value for --frames: " + value + ". Expected 'start-end' or 'frame'.");
        return std::nullopt;
    }
}

bool parse_render_option(const std::string& arg, const std::vector<std::string>& args, std::size_t& index, CliOptions& options, DiagnosticBag& diagnostics) {
    if (arg == "--cpp") {
        options.cpp_path = require_argument(args, index);
        if (options.cpp_path.empty()) diagnostics.add_error("cli.cpp_missing", "missing value for --cpp");
        return true;
    }
    if (arg == "--preset") {
        options.preset_id = require_argument(args, index);
        if (options.preset_id->empty()) diagnostics.add_error("cli.preset_missing", "missing value for --preset");
        return true;
    }
    if (arg == "--out") {
        options.output_override = require_argument(args, index);
        if (options.output_override.empty()) diagnostics.add_error("cli.out_missing", "missing value for --out");
        if (options.command == "preview" || options.command == "preview-frame") options.preview_output = options.output_override;
        return true;
    }
    if (arg == "--workers") {
        std::string val = require_argument(args, index);
        if (val.empty()) diagnostics.add_error("cli.workers_missing", "missing value for --workers");
        else options.worker_count = parse_worker_count(val, diagnostics);
        return true;
    }
    if (arg == "--frames") {
        std::string val = require_argument(args, index);
        if (val.empty()) diagnostics.add_error("cli.frames_missing", "missing value for --frames");
        else options.frame_range_override = parse_frame_range(val, diagnostics);
        return true;
    }
    if (arg == "--frame") {
        std::string val = require_argument(args, index);
        if (val.empty()) diagnostics.add_error("cli.frame_missing", "missing value for --frame");
        else {
            try { options.preview_frame_number = std::stoi(val); }
            catch (...) { diagnostics.add_error("cli.frame_invalid", "invalid value for --frame: " + val); }
        }
        return true;
    }
    if (arg == "--quality") {
        options.quality = require_argument(args, index);
        if (options.quality.empty()) diagnostics.add_error("cli.quality_missing", "missing value for --quality");
        return true;
    }
    return false;
}

bool parse_inspect_option(const std::string& arg, const std::vector<std::string>& args, std::size_t& index, CliOptions& options, DiagnosticBag& diagnostics) {
    if (arg == "--json") { options.json_output = true; return true; }
    if (arg == "--info") { options.inspect_include_info = true; return true; }
    if (arg == "--samples") {
        std::string val = require_argument(args, index);
        if (val.empty()) diagnostics.add_error("cli.samples_missing", "missing value for --samples");
        else {
            try {
                options.inspect_samples = std::stoi(val);
                if (options.inspect_samples <= 0) diagnostics.add_error("cli.samples_invalid", "--samples must be greater than zero");
                else options.samples_explicitly_set = true;
            } catch (...) { diagnostics.add_error("cli.samples_invalid", "invalid value for --samples: " + val); }
        }
        return true;
    }
    return false;
}

bool parse_metrics_option(const std::string& arg, const std::vector<std::string>& args, std::size_t& index, CliOptions& options, DiagnosticBag& diagnostics) {
    if (arg == "--input" && (options.command == "metrics" || options.command == "probe")) {
        options.metrics_input = require_argument(args, index);
        if (options.metrics_input.empty()) diagnostics.add_error("cli.input_missing", "missing value for --input");
        if (options.command == "probe") options.probe_input = options.metrics_input;
        return true;
    }
    if (arg == "--top") {
        std::string val = require_argument(args, index);
        if (val.empty()) diagnostics.add_error("cli.top_missing", "missing value for --top");
        else {
            try { options.metrics_top = std::stoi(val); }
            catch (...) { diagnostics.add_error("cli.top_invalid", "invalid value for --top: " + val); }
        }
        return true;
    }
    if (index == 1 && options.command == "metrics") {
        if (arg == "summary" || arg == "failures" || arg == "slowest") {
            options.metrics_command = arg;
            return true;
        }
    }
    return false;
}

bool parse_tool_option(const std::string& arg, const std::vector<std::string>& args, std::size_t& index, CliOptions& options, DiagnosticBag& diagnostics) {
    if (arg == "--inputs") {
        std::string val = require_argument(args, index);
        if (val.empty()) diagnostics.add_error("cli.inputs_missing", "missing value for --inputs");
        else {
            std::size_t start = 0;
            std::size_t end = val.find(',');
            while (end != std::string::npos) {
                options.concat_inputs.push_back(val.substr(start, end - start));
                start = end + 1;
                end = val.find(',', start);
            }
            options.concat_inputs.push_back(val.substr(start));
        }
        return true;
    }
    return false;
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

        // Remaining standalone options
        if (arg == "--library") {
            options.library_path = require_argument(args, index);
            if (options.library_path.empty()) result.diagnostics.add_error("cli.library_missing", "missing value for --library");
            continue;
        }
        if (arg == "--output-preset") {
            options.output_preset_id = require_argument(args, index);
            if (options.output_preset_id->empty()) result.diagnostics.add_error("cli.output_preset_missing", "missing value for --output-preset");
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
        
        // Output-presets subcommands
        if (index == 1 && options.command == "output-presets") {
            if (arg == "list" || arg == "info") { options.output_presets_command = arg; continue; }
            if (!arg.empty() && arg.front() != '-') {
                result.diagnostics.add_error("cli.output_presets_subcommand_invalid", "unknown output-presets subcommand: " + arg);
                return result;
            }
        }
        if (options.command == "output-presets" && options.output_presets_command == "info" && index == 2 && options.output_preset_name.empty()) {
            options.output_preset_name = arg;
            continue;
        }

        if (arg == "--version" || arg == "-v") { options.show_version = true; continue; }
        if (arg == "--all") { options.render_all_compositions = true; continue; }
        if (arg == "--cost-per-hour") {
            std::string val = require_argument(args, index);
            if (val.empty()) result.diagnostics.add_error("cli.cost_missing", "missing value for --cost-per-hour");
            else {
                try { options.machine_cost_per_hour = std::stod(val); }
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
