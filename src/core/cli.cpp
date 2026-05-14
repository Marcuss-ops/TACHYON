#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/core.h"
#include "cli/cli_internal.h"
#include "cli/command_registry.h"
#include "tachyon/runtime/registry/runtime_registry_bundle.h"
#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

namespace tachyon {

// Canonical registry bundle for CLI operations
static std::unique_ptr<runtime::RuntimeRegistryBundle> g_cli_bundle;

namespace {

void print_help(std::ostream& out) {
    out << "TACHYON " << version_string() << '\n';
    out << "Usage:\n";
    out << "  tachyon version\n";
    
    // Commands from registry
    auto commands = CommandRegistry::get_instance().get_commands();
    for (const auto& cmd : commands) {
        out << "  " << cmd.usage << '\n';
    }
}

} // namespace

int run_cli(int argc, char** argv) {
    const auto parsed = parse_cli_options(argc, argv);
    if (!parsed.value.has_value()) {
        print_diagnostics(parsed.diagnostics, std::cerr);
        return 1;
    }

    const CliOptions& options = *parsed.value;

    if (options.command == "version" || options.show_version) {
        std::cout << version_string() << '\n';
        return 0;
    }

    if (options.command.empty()) {
        print_help(std::cerr);
        return 1;
    }

    // Initialize unified runtime registry bundle
    if (!g_cli_bundle) {
        g_cli_bundle = std::make_unique<runtime::RuntimeRegistryBundle>(runtime::create_default_runtime_registry_bundle());
    }

    // Dispatch through registry
    CommandContext context{options, std::cout, std::cerr, *g_cli_bundle};
    auto result = CommandRegistry::get_instance().dispatch(context);

    if (result == CommandDispatchResult::NotFound) {
        print_help(std::cerr);
        return 1;
    }

    if (result == CommandDispatchResult::ValidationError) {
        return 1;
    }

    return (result == CommandDispatchResult::Success) ? 0 : 2;
}

} // namespace tachyon
