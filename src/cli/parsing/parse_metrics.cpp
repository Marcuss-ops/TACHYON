#include "parse_helpers.h"

namespace tachyon {

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

} // namespace tachyon
