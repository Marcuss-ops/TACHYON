#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/core/spec/cpp_scene_loader.h"
#include "tachyon/media/resolution/asset_resolution.h"
#include "tachyon/media/resolution/asset_path_utils.h"
#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/scene/builder.h"
#include "cli_internal.h"

#include <iostream>

namespace tachyon {

LoadSceneResult load_scene_for_cli(
    const SceneLoadOptions& options,
    SceneLoadMode mode,
    std::ostream& out,
    std::ostream& err
) {
    (void)mode;
    (void)err;

    LoadSceneResult result;
    result.success = false;

    if (options.preset_id.has_value()) {
        const auto& pid = *options.preset_id;

        auto bg = presets::BackgroundPresetRegistry::instance().create(pid, 1280, 720);
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
        out << "Loading C++ scene: " << options.cpp_path << "\n";

        auto load_result = CppSceneLoader::load_from_file(options.cpp_path);
        if (load_result.success && load_result.scene.has_value()) {
            LoadedSceneContext context;
            context.scene = std::move(load_result.scene.value());
            context.source_path = options.cpp_path;
            context.from_cpp = true;

            const auto resolved = resolve_assets(
                context.scene,
                tachyon::media::scene_asset_root(options.cpp_path)
            );

            if (resolved.value.has_value()) {
                context.assets = *resolved.value;
            }

            result.context = std::move(context);
            result.success = true;
            return result;
        }

        result.diagnostics.add_error(
            "cpp_load_failed",
            "Failed to load C++ scene: " + load_result.diagnostics
        );

        return result;
    }

    result.diagnostics.add_error(
        "no_scene_source",
        "No scene source provided. Use --cpp or --preset."
    );

    return result;
}

} // namespace tachyon
