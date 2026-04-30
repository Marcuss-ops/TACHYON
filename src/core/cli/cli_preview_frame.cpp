#include "cli_internal.h"

#include <ostream>

namespace tachyon {

bool run_preview_frame_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    (void)options;
    (void)out;
    err << "preview-frame command is not available in this build\n";
    return false;
}

} // namespace tachyon
