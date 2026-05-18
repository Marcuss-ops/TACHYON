#include "parse_bench.h"
#include "parse_helpers.h"

namespace tachyon {

bool parse_bench_option(const std::string& arg, const std::vector<std::string>& args, std::size_t& index, CliOptions& options, DiagnosticBag& diagnostics) {
    if (options.command != "bench") return false;

    if (arg == "--cpp") {
        options.bench.cpp_path = require_argument(args, index);
        options.cpp_path = options.bench.cpp_path;
        return true;
    }
    if (arg == "--preset") {
        options.bench.preset_id = require_argument(args, index);
        options.preset_id = options.bench.preset_id;
        return true;
    }
    if (arg == "--frames") {
        options.bench.frame_range = require_argument(args, index);
        return true;
    }
    if (arg == "--runs") {
        std::string val = require_argument(args, index);
        try { options.bench.runs = std::stoi(val); }
        catch (...) { diagnostics.add_error("cli.runs_invalid", "invalid value for --runs: " + val); }
        return true;
    }
    if (arg == "--warmup") {
        std::string val = require_argument(args, index);
        try { options.bench.warmup = std::stoi(val); }
        catch (...) { diagnostics.add_error("cli.warmup_invalid", "invalid value for --warmup: " + val); }
        return true;
    }
    if (arg == "--out") {
        options.bench.out_dir = require_argument(args, index);
        return true;
    }
    if (arg == "--format") {
        options.bench.formats = require_argument(args, index);
        return true;
    }
    if (arg == "--cache") {
        std::string val = require_argument(args, index);
        if (val == "off" || val == "false" || val == "0") {
            options.bench.cache = false;
        } else {
            options.bench.cache = true;
        }
        return true;
    }
    return false;
}

bool parse_plugin_option(const std::string& arg, const std::vector<std::string>& args, std::size_t& index, CliOptions& options, DiagnosticBag& diagnostics) {
    if (options.command != "plugins") return false;

    if (index == 1) {
        if (arg == "list" || arg == "validate" || arg == "reload") {
            options.plugins.subcommand = arg;
            return true;
        } else if (arg.front() != '-') {
            diagnostics.add_error("cli.plugins_subcommand_invalid", "unknown plugins subcommand: " + arg);
            return true;
        }
    }

    if (arg == "--dir") {
        options.plugins.dir = require_argument(args, index);
        return true;
    }
    if (arg == "--path") {
        options.plugins.path = require_argument(args, index);
        return true;
    }

    return false;
}

} // namespace tachyon
