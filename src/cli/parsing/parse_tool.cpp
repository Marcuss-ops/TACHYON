#include "parse_helpers.h"

namespace tachyon {

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

} // namespace tachyon
