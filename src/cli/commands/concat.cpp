#include "tachyon/runtime/registry/engine_registry.h"
#include "tachyon/backends/media_backend_bundle.h"
#include "cli/support/cli_internal.h"
#include "command_registry.h"
#include <iostream>

namespace tachyon {

bool run_concat_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::EngineRegistry& /*bundle*/) {
    if (options.concat.inputs.empty()) {
        err << "Error: --inputs is required for concat command\n";
        return false;
    }
    if (options.concat.output.empty()) {
        err << "Error: --out is required for concat command\n";
        return false;
    }

    core::media::ConcatConfig config;
    config.inputs = options.concat.inputs;
    config.output = options.concat.output;
    
    out << "Concatenating " << config.inputs.size() << " files into " << config.output.string() << "...\n";

    backends::MediaBackendBundle media_bundle;
    auto services = media_bundle.services();
    auto result = services.video_concat.concat_videos(config);
    if (!result.ok()) {
        err << "Error concatenating files: " << result.error->to_diagnostic_string() << "\n";
        return false;
    }

    out << "Concatenation successful.\n";
    return true;
}

CommandDescriptor make_concat_command() {
    return {
        "concat",
        "tachyon concat --inputs <file1,file2,...> --out <file>",
        [](const CliOptions& o, std::ostream& e) {
            if (o.concat.inputs.empty() || o.concat.output.empty()) {
                e << "--inputs and --out are required for concat\n";
                return false;
            }
            return true;
        },
        run_concat_command
    };
}

} // namespace tachyon
