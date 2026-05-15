#include "parse_helpers.h"
#include <exception>

namespace tachyon {

namespace {

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

bool parse_render_option(const std::string& arg, const std::vector<std::string>& args, std::size_t& index, CliOptions& options, DiagnosticBag& diagnostics) {
    if (arg == "--cpp") {
        options.cpp_path = require_argument(args, index);
        if (options.cpp_path.empty()) diagnostics.add_error("cli.cpp_missing", "missing value for --cpp");
        return true;
    }
    if (arg == "--preset") {
        options.preset_id = require_argument(args, index);
        if (options.preset_id->empty()) diagnostics.add_error("cli.preset_missing", "missing value for --preset");
        return true;
    }
    if (arg == "--out") {
        options.output_override = require_argument(args, index);
        if (options.output_override.empty()) diagnostics.add_error("cli.out_missing", "missing value for --out");
        if (options.command == "preview" || options.command == "preview-frame") options.preview_output = options.output_override;
        return true;
    }
    if (arg == "--workers") {
        std::string val = require_argument(args, index);
        if (val.empty()) diagnostics.add_error("cli.workers_missing", "missing value for --workers");
        else options.worker_count = parse_worker_count(val, diagnostics);
        return true;
    }
    if (arg == "--frames") {
        std::string val = require_argument(args, index);
        if (val.empty()) diagnostics.add_error("cli.frames_missing", "missing value for --frames");
        else options.frame_range_override = parse_frame_range(val, diagnostics);
        return true;
    }
    if (arg == "--frame") {
        std::string val = require_argument(args, index);
        if (val.empty()) diagnostics.add_error("cli.frame_missing", "missing value for --frame");
        else {
            try { options.preview_frame_number = std::stoi(val); }
            catch (...) { diagnostics.add_error("cli.frame_invalid", "invalid value for --frame: " + val); }
        }
        return true;
    }
    if (arg == "--quality") {
        options.quality = require_argument(args, index);
        if (options.quality.empty()) diagnostics.add_error("cli.quality_missing", "missing value for --quality");
        return true;
    }
    return false;
}

} // namespace tachyon
