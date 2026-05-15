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

// Forward declaration of the command manifest factory
std::vector<CommandDescriptor> make_all_commands();

void CommandRegistry::register_all_commands() {
    auto& reg = instance();
    if (!reg.m_commands.empty()) return;

    for (auto& cmd : make_all_commands()) {
        reg.register_command(std::move(cmd));
    }
}

} // namespace tachyon
