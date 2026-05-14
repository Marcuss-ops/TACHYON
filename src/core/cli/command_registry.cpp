#include "command_registry.h"
#include "cli_internal.h"
#include <algorithm>

namespace tachyon {

CommandRegistry& CommandRegistry::instance() {
    static CommandRegistry inst;
    return inst;
}

CommandRegistry::CommandRegistry() {
    // Register all built-in commands
    
    register_command({
        "validate",
        "tachyon validate --cpp <scene.cpp> [--job <file>]",
        [](const CliOptions& o, std::ostream& e) {
            if (o.cpp_path.empty() && !o.preset_id.has_value()) {
                e << "Either --cpp or --preset is required for validate\n";
                return false;
            }
            return true;
        },
        run_validate_command
    });

    register_command({
        "inspect",
        "tachyon inspect --cpp <scene.cpp> [--job <file>] [--json] [--info] [--samples <n>]",
        [](const CliOptions& o, std::ostream& e) {
            if (o.cpp_path.empty() && !o.preset_id.has_value()) {
                e << "Either --cpp or --preset is required for inspect\n";
                return false;
            }
            return true;
        },
        run_inspect_command
    });

    register_command({
        "motion-map",
        "tachyon motion-map --cpp <scene.cpp> [--preset <id>] [--json] [--samples <n>]",
        [](const CliOptions& o, std::ostream& e) {
            if (o.cpp_path.empty() && !o.preset_id.has_value()) {
                e << "Either --cpp or --preset is required for motion-map\n";
                return false;
            }
            return true;
        },
        run_motion_map_command
    });

    register_command({
        "render",
        "tachyon render --cpp <scene.cpp> --out <file> [--frames <s-e>] [--quality draft|high|production] [--workers <n>]\n"
        "        tachyon render --preset <id> --out <file> [--output-preset <name>]",
        [](const CliOptions& o, std::ostream& e) {
            if (o.cpp_path.empty() && !o.preset_id.has_value()) {
                e << "Either --cpp or --preset required for render\n";
                return false;
            }
            return true;
        },
        run_render_command
    });

    register_command({
        "output-presets",
        "tachyon output-presets list\n"
        "        tachyon output-presets info <name>",
        [](const CliOptions& o, std::ostream& e) {
            if (o.output_presets_command.empty()) {
                e << "Use output-presets list or output-presets info <name>\n";
                return false;
            }
            if (o.output_presets_command == "info" && o.output_preset_name.empty()) {
                e << "output-presets info requires a preset name\n";
                return false;
            }
            return true;
        },
        run_output_presets_command
    });

    register_command({
        "preview",
        "tachyon preview --cpp <scene.cpp> [--out <file.png>] [--frame <n>]\n"
        "        preview --preset <id>  [--out <file.png>] [--frame <n>]",
        [](const CliOptions& o, std::ostream& e) {
            if (o.cpp_path.empty() && !o.preset_id.has_value()) {
                e << "Either --cpp or --preset is required for preview\n";
                return false;
            }
            return true;
        },
        run_preview_command
    });

    register_command({
        "preview-frame",
        "tachyon preview-frame --cpp <scene.cpp>  --job <file> --frame <n> --out <file.png>",
        [](const CliOptions& o, std::ostream& e) {
            if (o.cpp_path.empty()) {
                e << "--cpp is required for preview-frame\n";
                return false;
            }
            if (o.job_path.empty() || !o.preview_frame_number.has_value() || o.preview_output.empty()) {
                e << "--job, --frame and --out are required for preview-frame\n";
                return false;
            }
            return true;
        },
        run_preview_frame_command
    });

    register_command({
        "watch",
        "tachyon watch --cpp <scene.cpp> --job <file> [--workers <n>]",
        nullptr,
        run_watch_command
    });

    register_command({
        "fetch-fonts",
        "tachyon fetch-fonts --family <name> [--weights <w1,w2,...>] [--subsets <s1,...>] [--dest <dir>] [--overwrite]",
        nullptr,
        run_fetch_fonts_command
    });

    register_command({
        "metrics",
        "tachyon metrics summary --input <file.jsonl> [--json]\n"
        "        metrics failures --input <file.jsonl>\n"
        "        metrics slowest --input <file.jsonl> [--top <n>]",
        [](const CliOptions& o, std::ostream& e) {
            if (o.metrics_input.empty()) {
                e << "--input is required for metrics command\n";
                return false;
            }
            return true;
        },
        run_metrics_command
    });

    register_command({
        "thumb",
        "tachyon thumb --cpp <scene.cpp> [--out <file.jpg>] [--frame <n>]\n"
        "        tachyon thumb --preset <id> [--out <file.jpg>] [--frame <n>]",
        [](const CliOptions& o, std::ostream& e) {
            if (o.cpp_path.empty() && !o.preset_id.has_value()) {
                e << "Either --cpp or --preset is required for thumb\n";
                return false;
            }
            return true;
        },
        run_thumb_command
    });

    register_command({
        "doctor",
        "tachyon doctor",
        nullptr,
        run_doctor_command
    });

    register_command({
        "probe",
        "tachyon probe --input <file> [--json]",
        [](const CliOptions& o, std::ostream& e) {
            if (o.probe_input.empty()) {
                e << "--input is required for probe\n";
                return false;
            }
            return true;
        },
        run_probe_command
    });

    register_command({
        "concat",
        "tachyon concat --inputs <file1,file2,...> --out <file>",
        [](const CliOptions& o, std::ostream& e) {
            if (o.concat_inputs.empty() || o.output_override.empty()) {
                e << "--inputs and --out are required for concat\n";
                return false;
            }
            return true;
        },
        run_concat_command
    });
}

void CommandRegistry::register_command(CommandDescriptor descriptor) {
    m_commands.push_back(std::move(descriptor));
}

const std::vector<CommandDescriptor>& CommandRegistry::commands() const {
    return m_commands;
}

const CommandDescriptor* CommandRegistry::find_command(const std::string& name) const {
    auto it = std::find_if(m_commands.begin(), m_commands.end(), [&](const CommandDescriptor& cmd) {
        return cmd.name == name;
    });
    return (it != m_commands.end()) ? &(*it) : nullptr;
}

} // namespace tachyon
