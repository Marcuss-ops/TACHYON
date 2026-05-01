#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/core/spec/cpp_scene_loader.h"

#include <iostream>

namespace tachyon {

void warn_legacy_json_scene(SceneLoadMode mode, std::ostream& err) {
    err << "WARNING: Legacy JSON scene loading is deprecated and will be removed in a future release.\n";
    err << "         Use C++ scene scripts (--cpp) instead.\n";
    if (mode == SceneLoadMode::Render) {
        err << "         Example: tachyon render --cpp scene.cpp\n";
    }
}

LoadSceneResult load_scene_for_cli(
    const SceneLoadOptions& options,
    SceneLoadMode mode,
    std::ostream& out,
    std::ostream& err
) {
    LoadSceneResult result;
    result.success = false;

    bool tried_cpp = false;
    bool tried_legacy_json = false;

    if (!options.cpp_path.empty()) {
        tried_cpp = true;
        out << "Loading C++ scene: " << options.cpp_path << "\n";

        auto load_result = CppSceneLoader::load_from_file(options.cpp_path);
        if (load_result.success && load_result.scene.has_value()) {
            LoadedSceneContext context;
            context.scene = std::move(load_result.scene.value());
            context.source_path = options.cpp_path;
            context.from_cpp = true;
            result.context = std::move(context);
            result.success = true;
            return result;
        } else {
            std::string error_msg = "Failed to load C++ scene: " + load_result.diagnostics;
            result.diagnostics.add_error("cpp_load_failed", std::move(error_msg));
        }
    }

    if (options.allow_legacy_json && !options.scene_path.empty()) {
        tried_legacy_json = true;

#ifdef TACHYON_ENABLE_LEGACY_JSON_SCENE
        warn_legacy_json_scene(mode, err);

        out << "Loading legacy JSON scene: " << options.scene_path << "\n";
        err << "ERROR: Legacy JSON scene loading is not yet implemented in this path.\n";
        err << "       Please use C++ scene scripts (--cpp) instead.\n";
        result.diagnostics.add_error("legacy_json_not_implemented", "Legacy JSON scene loading not implemented");
#else
        err << "ERROR: Legacy JSON scene support is disabled. Rebuild with TACHYON_ENABLE_LEGACY_JSON_SCENE=ON to enable.\n";
        err << "       Or use C++ scene scripts (--cpp) instead.\n";
        result.diagnostics.add_error("legacy_json_disabled", "Legacy JSON scene support is disabled");
#endif
    }

    if (!tried_cpp && !tried_legacy_json) {
        result.diagnostics.add_error("no_scene_path", "No scene path provided. Use --cpp for C++ scenes or --scene for JSON scenes.");
    }

    return result;
}

} // namespace tachyon
