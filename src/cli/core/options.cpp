#include "tachyon/core/cli_options.h"

#include <string>
#include <vector>

namespace tachyon {
namespace {

[[nodiscard]] bool is_help_flag(const std::string& value) {
    return value == "--help" || value == "-h";
}

std::string require_argument(const std::vector<std::string>& args, std::size_t& index) {
    if (index + 1 >= args.size()) {
        return {};
    }
    ++index;
    return args[index];
}

} // namespace

ParseResult<CliOptions> parse_cli_options(int argc, char** argv) {
    ParseResult<CliOptions> result;
    const std::vector<std::string> args(argv + 1, argv + argc);

    CliOptions options;
    if (args.empty()) {
        options.command = "help";
        options.help_requested = true;
        result.value = std::move(options);
        return result;
    }

    if (args[0] == "help" || is_help_flag(args[0])) {
        options.command = "help";
        options.help_requested = true;

        if (args.size() > 1 && !is_help_flag(args[1])) {
            options.command = args[1];
        }

        result.value = std::move(options);
        return result;
    }

    options.command = args[0];

    for (std::size_t index = 1; index < args.size(); ++index) {
        const std::string& arg = args[index];
        if (arg == "--scene") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                result.diagnostics.add_error("cli.scene_missing", "missing value for --scene");
                return result;
            }
            options.scene_path = value;
            continue;
        }
        if (arg == "--job") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                result.diagnostics.add_error("cli.job_missing", "missing value for --job");
                return result;
            }
            options.job_path = value;
            continue;
        }
        if (arg == "--out") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                result.diagnostics.add_error("cli.out_missing", "missing value for --out");
                return result;
            }
            options.output_override = value;
            continue;
        }
        if (arg == "--json") {
            options.json_output = true;
            continue;
        }
        if (is_help_flag(arg)) {
            options.help_requested = true;
            continue;
        }

        result.diagnostics.add_error("cli.flag_unknown", "unknown flag: " + arg, arg);
        return result;
    }

    result.value = std::move(options);
    return result;
}

} // namespace tachyon
