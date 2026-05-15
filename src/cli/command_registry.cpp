#include "command_registry.h"
#include "cli_internal.h"
#include <algorithm>

namespace tachyon {

CommandRegistry& CommandRegistry::instance() {
    static CommandRegistry inst;
    return inst;
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

void CommandRegistry::register_all_commands() {
    register_builtins();
    register_tools();
}

void CommandRegistry::register_builtins() {
    auto& reg = instance();
    if (!reg.find_command("render")) reg.register_command(make_render_command());
    if (!reg.find_command("preview")) reg.register_command(make_preview_command());
    if (!reg.find_command("preview-frame")) reg.register_command(make_preview_frame_command());
    if (!reg.find_command("validate")) reg.register_command(make_validate_command());
    if (!reg.find_command("inspect")) reg.register_command(make_inspect_command());
    if (!reg.find_command("version")) reg.register_command(make_version_command());
}

void CommandRegistry::register_tools() {
    auto& reg = instance();
    if (!reg.find_command("motion-map")) reg.register_command(make_motion_map_command());
    if (!reg.find_command("watch")) reg.register_command(make_watch_command());
    if (!reg.find_command("metrics")) reg.register_command(make_metrics_command());
    if (!reg.find_command("doctor")) reg.register_command(make_doctor_command());
    if (!reg.find_command("output-presets")) reg.register_command(make_output_presets_command());
    if (!reg.find_command("thumb")) reg.register_command(make_thumb_command());
    if (!reg.find_command("probe")) reg.register_command(make_probe_command());
    if (!reg.find_command("concat")) reg.register_command(make_concat_command());
    if (!reg.find_command("fetch-fonts")) reg.register_command(make_fetch_fonts_command());
    if (!reg.find_command("inspect-fonts")) reg.register_command(make_inspect_fonts_command());
}

} // namespace tachyon
