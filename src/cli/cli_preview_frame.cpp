#include "tachyon/runtime/registry/runtime_registry_bundle.h"
#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/runtime/execution/native_render.h"
#include "cli_internal.h"
#include "command_registry.h"
#include <ostream>

namespace tachyon {

bool run_preview_frame_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::RuntimeRegistryBundle& bundle) {
    return run_preview_internal(options, out, err, "PreviewFrame", bundle);
}

CommandDescriptor make_preview_frame_command() {
    return {
        "preview-frame",
        "tachyon preview-frame --cpp <scene.cpp>  --job <file> --frame <n> --out <file.png>",
        [](const CliOptions& o, std::ostream& e) {
            if (o.cpp_path.empty()) {
                e << "--cpp is required for preview-frame\n";
                return false;
            }
            if (o.inspect.job_path.empty() || !o.render.preview_frame_number.has_value() || o.render.preview_output.empty()) {
                e << "--job, --frame and --out are required for preview-frame\n";
                return false;
            }
            return true;
        },
        run_preview_frame_command
    };
}

} // namespace tachyon
