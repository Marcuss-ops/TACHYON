#include "cli_internal.h"
#include "command.h"
#include "tachyon/core/core.h"
#include <iostream>

namespace tachyon {

CommandDescriptor make_version_command() {
    return CommandDescriptor{
        "version",
        "version               - Print version information",
        nullptr, // No extra validation needed
        [](const CliOptions&, std::ostream& out, std::ostream&, runtime::RuntimeRegistryBundle&) {
            out << "TACHYON " << version_string() << "\n";
            return true;
        }
    };
}

} // namespace tachyon
