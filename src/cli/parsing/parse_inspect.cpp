#include "parse_helpers.h"

namespace tachyon {

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

} // namespace tachyon
