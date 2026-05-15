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

    /**
     * @brief Retrieves all registered commands.
     */
    const std::vector<CommandDescriptor>& commands() const;

    /**
     * @brief Finds a command by name.
     */
    const CommandDescriptor* find_command(const std::string& name) const;

    /**
     * @brief Registers all known commands (Builtins + Tools).
     */
    static void register_all_commands();

    /**
     * @brief Registers all built-in commands.
     * @deprecated Use register_all_commands()
     */
    static void register_builtins();

    /**
     * @brief Registers all tool-based commands.
     * @deprecated Use register_all_commands()
     */
    static void register_tools();

private:
    CommandRegistry() = default;
    std::vector<CommandDescriptor> m_commands;
};

} // namespace tachyon
