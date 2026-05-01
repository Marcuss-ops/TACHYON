#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/core/spec/cpp_scene_loader.h"
#include "tachyon/media/resolution/asset_resolution.h"
#include "tachyon/media/resolution/asset_path_utils.h"
#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/scene/builder.h"
#include "cli_internal.h"

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
    bool tried_preset = false;
    bool tried_legacy_json = false;

    if (options.preset_id.has_value()) {
        tried_preset = true;
        const auto& pid = *options.preset_id;
        
        auto bg = presets::build_background_preset(pid, 1280, 720);
        if (!bg) {
            result.diagnostics.add_error("preset_not_found", "Unknown preset: " + pid);
            return result;
        }

        SceneSpec scene = ::tachyon::scene::Composition("preset_render")
            .size(1280, 720)
            .duration(2.0)
            .fps(30)
            .layer("bg", [&](::tachyon::scene::LayerBuilder& l) {
                l = ::tachyon::scene::LayerBuilder(*bg);
            })
            .build_scene();

        LoadedSceneContext context;
        context.scene = std::move(scene);
        context.from_preset = true;
        result.context = std::move(context);
        result.success = true;
        return result;
    }

    if (!options.cpp_path.empty()) {
        tried_cpp = true;
        out << "Loading C++ scene: " << options.cpp_path << "\n";

        auto load_result = CppSceneLoader::load_from_file(options.cpp_path);
        if (load_result.success && load_result.scene.has_value()) {
            LoadedSceneContext context;
            context.scene = std::move(load_result.scene.value());
            context.source_path = options.cpp_path;
            context.from_cpp = true;

            const auto resolved = resolve_assets(context.scene, tachyon::media::scene_asset_root(options.cpp_path));
            if (resolved.value.has_value()) {
                context.assets = *resolved.value;
            }

            result.context = std::move(context);
            result.success = true;
            return result;
        } else {
            std::string error_msg = "Failed to load C++ scene: " + load_result.diagnostics;
            result.diagnostics.add_error("cpp_load_failed", std::move(error_msg));
        }
    }

    if (!options.scene_path.empty()) {
        tried_legacy_json = true;

        SceneSpec scene;
        AssetResolutionTable assets;
        if (!load_scene_context(options.scene_path, scene, assets, err)) {
            result.diagnostics.add_error("scene_load_failed", "Failed to load scene file: " + options.scene_path.string());
            return result;
        }

        LoadedSceneContext context;
        context.scene = std::move(scene);
        context.assets = std::move(assets);
        context.source_path = options.scene_path;
        context.from_legacy_json = options.allow_legacy_json;
        result.context = std::move(context);
        result.success = true;
        return result;
    }

    if (!tried_cpp && !tried_preset && !tried_legacy_json) {
        result.diagnostics.add_error("no_scene_path", "No scene path provided. Use --cpp, --preset, or --scene.");
    }

    return result;
}

} // namespace tachyon
