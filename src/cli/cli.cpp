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

namespace tachyon {

// Canonical registry bundle for CLI operations
static std::unique_ptr<runtime::RuntimeRegistryBundle> g_cli_bundle;

namespace {

void print_help(std::ostream& out) {
    out << "TACHYON " << version_string() << '\n';
    out << "Usage:\n";
    out << "  tachyon version\n";
    for (const auto& cmd : CommandRegistry::instance().commands()) {
        // Indent continuation lines
        std::string usage = cmd.usage;
        out << "  " << usage << '\n';
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

    // Initialize unified runtime registry bundle
    if (!g_cli_bundle) {
        g_cli_bundle = std::make_unique<runtime::RuntimeRegistryBundle>(runtime::create_default_runtime_registry_bundle());
    }

    // Dispatch through registry
    const auto* cmd = CommandRegistry::instance().find_command(options.command);
    if (cmd) {
        if (cmd->validate && !cmd->validate(options, std::cerr)) return 1;
        return cmd->handler(options, std::cout, std::cerr, *g_cli_bundle) ? 0 : 2;
    }

    print_help(std::cerr);
    return 1;
}

} // namespace tachyon
