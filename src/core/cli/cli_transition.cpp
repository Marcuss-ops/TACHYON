#include "cli_internal.h"

namespace tachyon {

bool run_transition_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& /*registry*/) {
    (void)options;
    out << "transition command is not implemented yet\n";
    err << "transition command is not implemented yet\n";
    return false;
}

} // namespace tachyon
