#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/media/resolution/asset_resolution.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"

#include <filesystem>
#include <optional>
#include <ostream>
#include <string>

namespace tachyon {

enum class SceneLoadMode {
    Render,
    Validate,
    Inspect,
    Preview,
    Watch
};

struct SceneLoadOptions {
    std::filesystem::path cpp_path;
    std::optional<std::string> preset_id;
};

struct LoadedSceneContext {
    SceneSpec scene;
    AssetResolutionTable assets;
    std::filesystem::path source_path;
    bool from_cpp{false};
    bool from_preset{false};
};

struct LoadSceneResult {
    bool success{false};
    std::optional<LoadedSceneContext> context;
    DiagnosticBag diagnostics;
};


LoadSceneResult load_scene_for_cli(
    const SceneLoadOptions& options,
    SceneLoadMode mode,
    std::ostream& out,
    std::ostream& err
);

} // namespace tachyon
