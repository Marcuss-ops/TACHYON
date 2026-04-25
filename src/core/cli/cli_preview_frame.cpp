#include "cli_internal.h"

namespace tachyon {

bool run_preview_frame_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    (void)options;
    out << "preview frame command is not implemented yet\n";
    err << "preview frame command is not implemented yet\n";
    return false;
}

} // namespace tachyon
