#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/spec/cpp_scene_loader.h"
#include "tachyon/runtime/execution/native_render.h"
#include "cli_internal.h"

#include <ostream>

namespace tachyon {

bool run_preview_frame_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    SceneSpec scene;
    AssetResolutionTable assets;

    if (!options.cpp_path.empty()) {
        if (!options.json_output)
            out << "[NativeLoader] Compiling scene from " << options.cpp_path.string() << "...\n";
        const auto result = CppSceneLoader::load_from_file(options.cpp_path);
        if (!result.success) {
            err << "C++ Scene Loader failed:\n" << result.diagnostics << "\n";
            return false;
        }
        scene = std::move(*result.scene);
    } else if (!options.scene_path.empty()) {
        err << "WARNING: Previewing from JSON scene files is DEPRECATED. Use --cpp instead.\n";
        if (!load_scene_context(options.scene_path, scene, assets, err)) return false;
    } else {
        err << "Either --cpp or --scene is required for preview-frame\n";
        return false;
    }

    if (scene.compositions.empty()) {
        err << "Scene has no compositions.\n";
        return false;
    }

    std::string composition_id = scene.compositions.front().id;
    std::int64_t frame = options.preview_frame_number.has_value() ? *options.preview_frame_number : 0;
    std::filesystem::path output = options.preview_output;

    const bool success = NativeRenderer::render_still(scene, composition_id, frame, output);
    if (!success) {
        err << "Preview render failed.\n";
    } else if (options.json_output) {
        out << "{\"status\": \"ok\", \"output\": \"" << output.string() << "\"}\n";
    } else {
        out << "[PreviewFrame] Wrote " << output.string() << "\n";
    }

    return success;
}

} // namespace tachyon
