#include "tachyon/runtime/registry/runtime_registry_bundle.h"
#include "cli_internal.h"
#include "command_registry.h"
#include "tachyon/media/video_concat.h"
#include <iostream>

namespace tachyon {

bool run_concat_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::RuntimeRegistryBundle& /*bundle*/) {
    if (options.media.concat_inputs.empty()) {
        err << "Error: --inputs is required for concat command\n";
        return false;
    }
    if (options.media.concat_output.empty()) {
        err << "Error: --out is required for concat command\n";
        return false;
    }

    media::ConcatConfig config;
    config.inputs = options.media.concat_inputs;
    config.output = options.media.concat_output;
    
    out << "Concatenating " << config.inputs.size() << " files into " << config.output.string() << "...\n";

    auto result = media::VideoConcat::concat_videos(config);
    if (!result.ok()) {
        err << "Error concatenating files: " << result.error->to_diagnostic_string() << "\n";
        return false;
    }

    out << "Concatenation successful.\n";
    return true;
}

REGISTER_COMMAND(
    "concat",
    "tachyon concat --inputs <file1,file2,...> --out <file>",
    [](const CliOptions& o, std::ostream& e) {
        if (o.media.concat_inputs.empty() || o.media.concat_output.empty()) {
            e << "--inputs and --out are required for concat\n";
            return false;
        }
        return true;
    },
    run_concat_command
);

} // namespace tachyon
