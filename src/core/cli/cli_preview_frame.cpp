#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/runtime/execution/native_render.h"
#include "cli_internal.h"

#include <ostream>

namespace tachyon {

bool run_preview_frame_command(const CliOptions& options, std::ostream& out, std::ostream& err, TransitionRegistry& registry) {
    return run_preview_internal(options, out, err, "PreviewFrame", registry);
}

} // namespace tachyon
