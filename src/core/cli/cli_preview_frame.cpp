#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/runtime/execution/native_render.h"

#include <ostream>

#include "command.h"

namespace tachyon {

bool run_preview_frame_command(const CommandContext& context) {
    return run_preview_internal(context, "PreviewFrame");
}

} // namespace tachyon
