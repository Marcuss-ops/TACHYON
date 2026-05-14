#pragma once

#include "tachyon/core/cli_options.h"
#include "tachyon/runtime/registry/runtime_registry_bundle.h"
#include <iostream>
#include <string>
#include <functional>

namespace tachyon {

/**
 * Execution context for a CLI command.
 * Aggregates all dependencies needed by command handlers.
 */
struct CommandContext {
    const CliOptions& options;
    std::ostream& out;
    std::ostream& err;
    runtime::RuntimeRegistryBundle& runtime;
};

/**
 * Descriptor for a CLI command.
 */
struct CommandDescriptor {
    std::string name;
    std::string usage;
    
    // Returns false (+ prints to context.err) when required args are missing.
    std::function<bool(const CommandContext&)> validate;
    
    // Executes the command logic. Returns true on success.
    std::function<bool(const CommandContext&)> run;
};

} // namespace tachyon
