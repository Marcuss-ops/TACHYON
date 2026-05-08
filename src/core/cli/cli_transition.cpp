#include "cli_internal.h"
#include "tachyon/renderer3d/modifiers/modifier3d_registry.h"

namespace tachyon {

bool run_transition_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& /*registry*/, renderer3d::Modifier3DRegistry& /*modifier_registry*/) {
    (void)options;
    out << "transition command is not implemented yet\n";
    err << "transition command is not implemented yet\n";
    return false;
}

} // namespace tachyon
