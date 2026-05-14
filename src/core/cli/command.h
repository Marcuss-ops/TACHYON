#pragma once
#include "tachyon/core/cli_options.h"
#include <functional>
#include <string>
#include <ostream>

namespace tachyon {

namespace runtime { struct RuntimeRegistryBundle; }

/**
 * @brief Descriptor for a CLI command.
 */
struct CommandDescriptor {
    std::string name;
    std::string usage;
    
    // Returns false (+ prints to err) when required args are missing.
    std::function<bool(const CliOptions&, std::ostream&)> validate;
    
    // Executes the command.
    std::function<bool(const CliOptions&, std::ostream&, std::ostream&, runtime::RuntimeRegistryBundle&)> handler;
};

} // namespace tachyon
