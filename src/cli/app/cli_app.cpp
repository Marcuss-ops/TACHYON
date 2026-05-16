#include "cli_app.h"
#include "tachyon/core/core.h"
#include "cli/support/cli_internal.h"
#include "cli/commands/command.h"
#include "cli/commands/command_registry.h"
#include "tachyon/runtime/registry/engine_registry.h"
#include "tachyon/diagnostics/trace_session.h"
#include <iostream>

namespace tachyon {

CliApp::CliApp() = default;
CliApp::~CliApp() = default;

void CliApp::register_commands() {
    CommandRegistry::register_all_commands();
}

void CliApp::print_help(std::ostream& out) {
    out << "TACHYON " << version_string() << '\n';
    out << "Usage:\n";
    for (const auto& cmd : CommandRegistry::instance().commands()) {
        out << "  " << cmd.usage << '\n';
    }
}

int CliApp::run(int argc, char** argv) {
    // 1. Register all commands
    register_commands();

    // 2. Parse global options and command name
    const auto parsed = parse_cli_options(argc, argv);
    if (!parsed.value.has_value()) {
        print_diagnostics(parsed.diagnostics, std::cerr);
        return 1;
    }

    const CliOptions& options = *parsed.value;

    if (options.show_version) {
        std::cout << version_string() << '\n';
        return 0;
    }

    if (options.command.empty()) {
        print_help(std::cerr);
        return 1;
    }

    // 3. Initialize unified engine registry
    if (!m_registry.has_value()) {
        m_registry = runtime::create_default_engine_registry();
    }

    // 4. Initialize tracing if requested
    if (!options.trace_json.empty()) {
        diagnostics::start_json_trace(options.trace_json.string());
    }

    // 5. Dispatch through registry
    int exit_code = 1;
    const auto* cmd = CommandRegistry::instance().find_command(options.command);
    if (cmd) {
        if (cmd->validate && !cmd->validate(options, std::cerr)) {
            exit_code = 1;
        } else {
            exit_code = cmd->handler(options, std::cout, std::cerr, *m_registry) ? 0 : 2;
        }
    } else {
        std::cerr << "Unknown command: " << options.command << "\n\n";
        print_help(std::cerr);
        exit_code = 1;
    }

    // 6. Finalize tracing
    if (!options.trace_json.empty()) {
        diagnostics::stop_trace();
    }

    return exit_code;
}

} // namespace tachyon
