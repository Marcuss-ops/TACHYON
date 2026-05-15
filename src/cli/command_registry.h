#pragma once
#include "command.h"
#include <vector>
#include <memory>

namespace tachyon {

/**
 * @brief Central registry for all Tachyon CLI commands.
 */
class CommandRegistry {
public:
    static CommandRegistry& instance();

    /**
     * @brief Registers a new command.
     */
    void register_command(CommandDescriptor descriptor);

    static void add(CommandDescriptor descriptor) {
        instance().register_command(std::move(descriptor));
    }

    /**
     * @brief Retrieves all registered commands.
     */
    const std::vector<CommandDescriptor>& commands() const;

    /**
     * @brief Finds a command by name.
     */
    const CommandDescriptor* find_command(const std::string& name) const;

private:
    CommandRegistry() = default;
    std::vector<CommandDescriptor> m_commands;
};

/**
 * @brief Helper for static registration of commands.
 */
struct CommandRegistrar {
    CommandRegistrar(CommandDescriptor descriptor) {
        CommandRegistry::add(std::move(descriptor));
    }
};

#define REGISTER_COMMAND(name_str, usage_str, validate_fn, handler_fn) \
    static tachyon::CommandRegistrar g_registrar_##handler_fn({name_str, usage_str, validate_fn, handler_fn});

} // namespace tachyon
