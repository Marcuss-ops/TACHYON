#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/output/output_presets.h"
#include "cli_internal.h"

#include <iomanip>
#include <iostream>
#include <string>

namespace tachyon {

namespace {

void print_preset_info(const tachyon::output::OutputPreset& preset, const std::string& name, std::ostream& out) {
    out << name << '\n';
    out << "  codec: " << preset.codec << '\n';
    out << "  pixel_fmt: " << preset.pixel_fmt << '\n';
    out << "  container: " << preset.container << '\n';
    out << "  class: " << preset.class_name << '\n';
    out << "  alpha_mode: " << preset.alpha_mode << '\n';
    out << "  rate_control_mode: " << preset.rate_control_mode << '\n';
    out << "  crf: " << preset.crf << '\n';
    out << "  color_space: " << preset.color_space << '\n';
    out << "  color_range: " << preset.color_range << '\n';
    out << "  faststart: " << (preset.faststart ? "true" : "false") << '\n';
    out << "  width: " << preset.width << '\n';
    out << "  height: " << preset.height << '\n';
}

} // namespace

bool run_output_presets_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& /*registry*/) {
    const auto command = options.output_presets_command;
    if (command.empty()) {
        err << "Use `tachyon output-presets list` or `tachyon output-presets info <id>`\n";
        return false;
    }

    if (command == "list") {
        const auto ids = output::list_presets();
        for (const auto& id : ids) {
            out << id << '\n';
        }
        return true;
    }

    if (command == "info") {
        if (options.output_preset_name.empty()) {
            err << "Missing preset id. Use `tachyon output-presets info <id>`\n";
            return false;
        }

        const auto* preset = output::find_preset(options.output_preset_name);
        if (preset == nullptr) {
            err << "Unknown output preset: " << options.output_preset_name << '\n';
            return false;
        }

        print_preset_info(*preset, options.output_preset_name, out);
        return true;
    }

    err << "Unknown output-presets subcommand: " << command << '\n';
    return false;
}

} // namespace tachyon
