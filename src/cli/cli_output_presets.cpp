#include "tachyon/runtime/registry/runtime_registry_bundle.h"
#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/output/output_presets.h"
#include "cli_internal.h"
#include "command_registry.h"
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

bool run_output_presets_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::RuntimeRegistryBundle& /*bundle*/) {
    const auto command = options.output_presets.command;
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
        if (options.output_presets.name.empty()) {
            err << "Missing preset id. Use `tachyon output-presets info <id>`\n";
            return false;
        }

        const auto* preset = output::find_preset(options.output_presets.name);
        if (preset == nullptr) {
            err << "Unknown output preset: " << options.output_presets.name << '\n';
            return false;
        }

        print_preset_info(*preset, options.output_presets.name, out);
        return true;
    }

    err << "Unknown output-presets subcommand: " << command << '\n';
    return false;
}

CommandDescriptor make_output_presets_command() {
    return {
        "output-presets",
        "tachyon output-presets list\n"
        "        tachyon output-presets info <name>",
        [](const CliOptions& o, std::ostream& e) {
            if (o.output_presets.command.empty()) {
                e << "Use output-presets list or output-presets info <name>\n";
                return false;
            }
            if (o.output_presets.command == "info" && o.output_presets.name.empty()) {
                e << "output-presets info requires a preset name\n";
                return false;
            }
            return true;
        },
        run_output_presets_command
    };
}

} // namespace tachyon
