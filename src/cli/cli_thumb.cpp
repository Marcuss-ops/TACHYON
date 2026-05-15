#include "tachyon/runtime/registry/runtime_registry_bundle.h"
#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/runtime/execution/native_render.h"
#include "cli_internal.h"
#include "command_registry.h"
#include <ostream>
#include <string>
#include <filesystem>
#include "tachyon/runtime/registry/runtime_registry_bundle.h"

namespace tachyon {

namespace {
std::string make_default_thumb_path(const std::string& cpp_path) {
    namespace fs = std::filesystem;
    fs::path p(cpp_path);
    fs::path out_dir = fs::path("output");
    if (!fs::exists(out_dir)) {
        fs::create_directories(out_dir);
    }
    return (out_dir / (p.stem().string() + ".thumb.jpg")).string();
}
} // namespace

bool run_thumb_command(const CliOptions& options, std::ostream& out, std::ostream& err, runtime::RuntimeRegistryBundle& bundle) {
    if (options.cpp_path.empty() && !options.preset_id.has_value()) {
        err << "Either --cpp or --preset is required for thumb\n";
        return false;
    }

    std::string cpp_path = options.cpp_path.string();
    if (options.preset_id.has_value()) {
        cpp_path = "preset:" + *options.preset_id;
    }

    std::string output_path;
    int frame_number = options.preview_frame_number.has_value()
                           ? *options.preview_frame_number
                           : 90; // Default to frame 90 (about 3 seconds at 30fps)

    const std::string source_name = options.cpp_path.empty()
                                        ? *options.preset_id
                                        : options.cpp_path.string();

    if (options.preview_output.empty()) {
        output_path = make_default_thumb_path(source_name);
    } else {
        output_path = options.preview_output.string();
    }

    out << "Generating thumbnail from frame " << frame_number << "...\n";
    out << "Output: " << output_path << "\n";

    // Build a minimal scene spec for thumbnail
    SceneLoadOptions load_opts;
    load_opts.cpp_path = options.cpp_path;
    load_opts.preset_id = options.preset_id;

    auto load_result = load_scene_for_cli(load_opts, SceneLoadMode::Preview, out, err);
    if (!load_result.success) {
        return false;
    }

    // Render single frame as thumbnail (JPEG)
    // This would call the actual render pipeline with single frame + JPEG output
    // For now, we use the preview infrastructure
    return run_preview_internal(options, out, err, "Thumbnail", bundle);
}

REGISTER_COMMAND(
    "thumb",
    "tachyon thumb --cpp <scene.cpp> [--out <file.jpg>] [--frame <n>]\n"
    "        tachyon thumb --preset <id> [--out <file.jpg>] [--frame <n>]",
    [](const CliOptions& o, std::ostream& e) {
        if (o.cpp_path.empty() && !o.preset_id.has_value()) {
            e << "Either --cpp or --preset is required for thumb\n";
            return false;
        }
        return true;
    },
    run_thumb_command
);

} // namespace tachyon
