#include "parse_inspect.h"
#include "parse_helpers.h"

namespace tachyon {

bool parse_inspect_option(const std::string& arg, const std::vector<std::string>& args, std::size_t& index, CliOptions& options, DiagnosticBag& diagnostics) {
    if (arg == "--cpp") {
        options.cpp_path = require_argument(args, index);
        if (options.cpp_path.empty()) diagnostics.add_error("cli.cpp_missing", "missing value for --cpp");
        return true;
    }
    if (arg == "--job") {
        options.inspect.job_path = require_argument(args, index);
        if (options.inspect.job_path.empty()) diagnostics.add_error("cli.job_missing", "missing value for --job");
        return true;
    }
    if (arg == "--json") { options.inspect.json_output = true; return true; }
    if (arg == "--info") { options.inspect.include_info = true; return true; }
    if (arg == "--samples") {
        std::string val = require_argument(args, index);
        if (val.empty()) diagnostics.add_error("cli.samples_missing", "missing value for --samples");
        else {
            try {
                options.inspect.samples = std::stoi(val);
                if (options.inspect.samples <= 0) diagnostics.add_error("cli.samples_invalid", "--samples must be greater than zero");
                else options.inspect.samples_explicitly_set = true;
            } catch (...) { diagnostics.add_error("cli.samples_invalid", "invalid value for --samples: " + val); }
        }
        return true;
    }
    return false;
}

} // namespace tachyon
