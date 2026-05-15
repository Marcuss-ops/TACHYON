#include "parse_metrics.h"
#include "parse_helpers.h"

namespace tachyon {

bool parse_metrics_option(const std::string& arg, const std::vector<std::string>& args, std::size_t& index, CliOptions& options, DiagnosticBag& diagnostics) {
    if (arg == "--input" && (options.command == "metrics" || options.command == "probe")) {
        if (options.command == "metrics") {
            options.metrics.input = require_argument(args, index);
            if (options.metrics.input.empty()) diagnostics.add_error("cli.input_missing", "missing value for --input");
        } else {
            options.media.probe_input = require_argument(args, index);
            if (options.media.probe_input.empty()) diagnostics.add_error("cli.input_missing", "missing value for --input");
        }
        return true;
    }
    if (arg == "--top") {
        std::string val = require_argument(args, index);
        if (val.empty()) diagnostics.add_error("cli.top_missing", "missing value for --top");
        else {
            try { options.metrics.top = std::stoi(val); }
            catch (...) { diagnostics.add_error("cli.top_invalid", "invalid value for --top: " + val); }
        }
        return true;
    }
    if (arg == "--json" && (options.command == "metrics" || options.command == "probe")) {
        if (options.command == "metrics") options.metrics.json_output = true;
        else options.media.json_output = true;
        return true;
    }
    if (index == 1 && options.command == "metrics") {
        if (arg == "summary" || arg == "failures" || arg == "slowest") {
            options.metrics.subcommand = arg;
            return true;
        }
    }
    return false;
}

} // namespace tachyon
