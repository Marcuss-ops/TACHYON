#include "tachyon/runtime/registry/runtime_registry_bundle.h"
#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/runtime/execution/native_render.h"
#include "cli_internal.h"

#include <ostream>

namespace tachyon {
#include "tachyon/runtime/registry/runtime_registry_bundle.h"

bool run_preview_frame_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::RuntimeRegistryBundle& bundle) {
    return run_preview_internal(options, out, err, "PreviewFrame", bundle);
}

} // namespace tachyon
