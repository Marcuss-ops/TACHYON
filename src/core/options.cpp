#include "tachyon/core/cli_options.h"
#include <exception>
#include <string>
#include <vector>
#include <algorithm>

namespace tachyon {
namespace {

std::string require_argument(const std::vector<std::string>& args, std::size_t& index) {
    if (index + 1 >= args.size()) return {};
    return args[++index];
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
        
        // Core Flags
        if (arg == "--cpp") { options.cpp_path = require_argument(args, index); continue; }
        if (arg == "--preset") { options.preset_id = require_argument(args, index); continue; }
        if (arg == "--out") { options.output_override = require_argument(args, index); continue; }
        if (arg == "--workers") { 
            auto val = require_argument(args, index);
            if (!val.empty()) options.worker_count = std::stoull(val); 
            continue; 
        }
        if (arg == "--frames") {
            auto val = require_argument(args, index);
            // Simple parsing for now, or use a helper
            const auto dash = val.find('-');
            if (dash != std::string::npos) {
                options.frame_range_override = FrameRange{std::stoll(val.substr(0, dash)), std::stoll(val.substr(dash+1))};
            }
            continue;
        }
        if (arg == "--frame") {
            auto val = require_argument(args, index);
            if (!val.empty()) options.preview_frame_number = std::stoi(val);
            continue;
        }
        if (arg == "--quality") { options.quality = require_argument(args, index); continue; }
        if (arg == "--json") { options.json_output = true; continue; }
        if (arg == "--all") { options.render_all_compositions = true; continue; }
        if (arg == "--version" || arg == "-v") { options.show_version = true; continue; }

        if (arg == "--inputs") {
            auto val = require_argument(args, index);
            std::size_t start = 0;
            std::size_t end = val.find(',');
            while (end != std::string::npos) {
                options.list_values["inputs"].push_back(val.substr(start, end - start));
                start = end + 1;
                end = val.find(',', start);
            }
            options.list_values["inputs"].push_back(val.substr(start));
            continue;
        }

        // generic option mapping for specialized commands
        if (arg.size() > 2 && arg.substr(0, 2) == "--") {
            std::string key = arg.substr(2);
            std::string value = require_argument(args, index);
            options.values[key] = value;
            continue;
        }

        // Subcommands / Positional
        if (index == 1) {
            options.values["subcommand"] = arg;
        } else {
            options.list_values["positional"].push_back(arg);
        }
    }

    result.value = std::move(options);
    return result;
}

} // namespace tachyon
