#pragma once

#include "tachyon/core/cli_options.h"
#include "tachyon/runtime/registry/engine_registry.h"
#include <memory>
#include <iostream>
#include <optional>

namespace tachyon {

/**
 * @brief Encapsulates the Tachyon CLI application state and execution.
 */
class CliApp {
public:
    CliApp();
    ~CliApp();

    /**
     * @brief Main entry point for the CLI.
     */
    int run(int argc, char** argv);

private:
    void print_help(std::ostream& out);
    void register_commands();

    std::optional<runtime::EngineRegistry> m_registry;
};

} // namespace tachyon
