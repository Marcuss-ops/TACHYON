#pragma once

#include "command.h"
#include <vector>
#include <memory>
#include <optional>
#include <iostream>

namespace tachyon {

enum class CommandDispatchResult {
    Success,
    NotFound,
    ValidationError,
    ExecutionError
};

class CommandRegistry {
public:
    static CommandRegistry& get_instance();

    void register_command(CommandDescriptor descriptor);
    
    // Dispatches to the command specified in context.options.command.
    CommandDispatchResult dispatch(const CommandContext& context) const;
    
    void print_help(std::ostream& out) const;

    const std::vector<CommandDescriptor>& get_commands() const { return commands_; }

private:
    std::vector<CommandDescriptor> commands_;
};

/**
 * Macro to register a command at static initialization time.
 * Note: Use this with caution regarding initialization order.
 */
#define TACHYON_REGISTER_COMMAND(descriptor) \
    static const bool _registered_##name = []() { \
        CommandRegistry::get_instance().register_command(descriptor); \
        return true; \
    }();

} // namespace tachyon
