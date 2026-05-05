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
        if (arg == "--catalog" || arg == "--library") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                result.diagnostics.add_error("cli.catalog_missing", "missing value for " + arg);
                return result;
            }
            options.catalog_path = value;
            continue;
        }
        if (arg == "--transition") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                result.diagnostics.add_error("cli.transition_missing", "missing value for --transition");
                return result;
            }
            options.transition_id = value;
            continue;
        }
        if (arg == "--out") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                result.diagnostics.add_error("cli.out_missing", "missing value for --out");
                return result;
            }
            options.output_override = value;
            if (options.command == "preview" || options.command == "preview-frame") {
                options.preview_output = value;
            }
            continue;
        }
        if (arg == "--preset") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                result.diagnostics.add_error("cli.preset_missing", "missing value for --preset");
                return result;
            }
            options.preset_id = value;
            continue;
        }
        if (arg == "--workers") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                result.diagnostics.add_error("cli.workers_missing", "missing value for --workers");
                return result;
            }
            options.worker_count = parse_worker_count(value, result.diagnostics);
            continue;
        }
        if (arg == "--memory-budget-mb") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                result.diagnostics.add_error("cli.memory_budget_missing", "missing value for --memory-budget-mb");
                return result;
            }
            const auto parsed_budget = parse_memory_budget_mb(value, result.diagnostics);
            if (parsed_budget.has_value()) {
                options.memory_budget_bytes = (*parsed_budget) * 1024ULL * 1024ULL;
            }
            continue;
        }
        if (arg == "--output-dir") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                result.diagnostics.add_error("cli.output_dir_missing", "missing value for --output-dir");
                return result;
            }
            options.output_dir = value;
            continue;
        }
        if (arg == "--frames") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                result.diagnostics.add_error("cli.frames_missing", "missing value for --frames");
                return result;
            }
            options.frame_range_override = parse_frame_range(value, result.diagnostics);
            continue;
        }
        if (arg == "--frame") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                result.diagnostics.add_error("cli.frame_missing", "missing value for --frame");
                return result;
            }
            try {
                options.preview_frame_number = std::stoi(value);
            } catch (const std::exception&) {
                result.diagnostics.add_error("cli.frame_invalid", "invalid value for --frame: " + value);
                return result;
            }
            continue;
        }
        if (arg == "--cpp") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                result.diagnostics.add_error("cli.cpp_missing", "missing value for --cpp");
                return result;
            }
            options.cpp_path = value;
            continue;
        }
        if (arg == "--quality") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                result.diagnostics.add_error("cli.quality_missing", "missing value for --quality");
                return result;
            }
            options.quality = value;
            continue;
        }
        if (arg == "--json") {
            options.json_output = true;
            continue;
        }
        if (arg == "--info") {
            options.inspect_include_info = true;
            continue;
        }
        if (arg == "--samples") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                result.diagnostics.add_error("cli.samples_missing", "missing value for --samples");
                return result;
            }
            try {
                options.inspect_samples = std::stoi(value);
                if (options.inspect_samples <= 0) {
                    result.diagnostics.add_error("cli.samples_invalid", "--samples must be greater than zero");
                    return result;
                }
                options.samples_explicitly_set = true;
            } catch (const std::exception&) {
                result.diagnostics.add_error("cli.samples_invalid", "invalid value for --samples: " + value);
                return result;
            }
            continue;
        }
        if (arg == "--version" || arg == "-v") {
            options.show_version = true;
            continue;
        }

        result.diagnostics.add_error("cli.flag_unknown", "unknown flag: " + arg, arg);
        return result;
    }

    if (result.diagnostics.ok()) {
        result.value = std::move(options);
    }
    return result;
}

} // namespace tachyon
