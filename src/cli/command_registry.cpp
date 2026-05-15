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

void CommandRegistry::register_builtins() {
    auto& reg = instance();
    reg.register_command(make_render_command());
    reg.register_command(make_preview_command());
    reg.register_command(make_preview_frame_command());
    reg.register_command(make_validate_command());
    reg.register_command(make_inspect_command());
    reg.register_command(make_version_command());
}

void CommandRegistry::register_tools() {
    auto& reg = instance();
    reg.register_command(make_motion_map_command());
    reg.register_command(make_watch_command());
    reg.register_command(make_metrics_command());
    reg.register_command(make_doctor_command());
    reg.register_command(make_output_presets_command());
    reg.register_command(make_thumb_command());
    reg.register_command(make_probe_command());
    reg.register_command(make_concat_command());
    reg.register_command(make_fetch_fonts_command());
    reg.register_command(make_inspect_fonts_command());
}

} // namespace tachyon
