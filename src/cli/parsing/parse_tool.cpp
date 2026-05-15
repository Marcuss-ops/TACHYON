#include "parse_tool.h"
#include "parse_helpers.h"

namespace tachyon {

bool parse_tool_option(const std::string& arg, const std::vector<std::string>& args, std::size_t& index, CliOptions& options, DiagnosticBag& diagnostics) {
    // Concat inputs
    if (arg == "--inputs") {
        std::string val = require_argument(args, index);
        if (val.empty()) diagnostics.add_error("cli.inputs_missing", "missing value for --inputs");
        else {
            std::size_t start = 0;
            std::size_t end = val.find(',');
            while (end != std::string::npos) {
                options.concat.inputs.push_back(val.substr(start, end - start));
                start = end + 1;
                end = val.find(',', start);
            }
            options.concat.inputs.push_back(val.substr(start));
        }
        return true;
    }
    
    // Concat output
    if (arg == "--out" && options.command == "concat") {
        options.concat.output = require_argument(args, index);
        return true;
    }

    // Font options
    if (arg == "--family") { options.fonts.family = require_argument(args, index); return true; }
    if (arg == "--dest") { options.fonts.dest = require_argument(args, index); return true; }
    if (arg == "--overwrite") { options.fonts.overwrite = true; return true; }
    
    // Thumb options
    if (arg == "--frame" && options.command == "thumb") {
        std::string val = require_argument(args, index);
        try { options.thumb.frame = std::stoi(val); }
        catch (...) { diagnostics.add_error("cli.frame_invalid", "invalid value for --frame: " + val); }
        return true;
    }
    
    if (arg == "--out" && options.command == "thumb") {
        options.thumb.output = require_argument(args, index);
        return true;
    }

    // Probe options
    if (arg == "--input" && options.command == "probe") {
        options.probe.input = require_argument(args, index);
        return true;
    }
    
    if (arg == "--json" && options.command == "probe") {
        options.probe.json_output = true;
        return true;
    }

    return false;
}

} // namespace tachyon
