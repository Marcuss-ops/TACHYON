#include "tachyon/runtime/registry/runtime_registry_bundle.h"
#include "cli_internal.h"
#include "command_registry.h"
#include "tachyon/media/video_concat.h"
#include <iostream>

namespace tachyon {

bool run_concat_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::RuntimeRegistryBundle& /*bundle*/) {
    if (options.concat_inputs.empty()) {
        err << "Error: --inputs is required for concat command\n";
#include "tachyon/runtime/registry/runtime_registry_bundle.h"
        return false;
    }
    if (options.output_override.empty()) {
        err << "Error: --out is required for concat command\n";
        return false;
    }

    media::ConcatConfig config;
    config.inputs = options.concat_inputs;
    config.output = options.output_override;
    
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
        if (o.concat_inputs.empty() || o.output_override.empty()) {
            e << "--inputs and --out are required for concat\n";
            return false;
        }
        return true;
    },
    run_concat_command
);

} // namespace tachyon
