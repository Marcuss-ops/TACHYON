#include "tachyon/core/cli.h"
#include "tachyon/core/report.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/media/resolution/asset_resolution.h"
#include <iostream>
#include <filesystem>

namespace tachyon {

void print_diagnostics(const DiagnosticBag& diagnostics, std::ostream& out) {
    for (const auto& diagnostic : diagnostics.diagnostics) {
        out << diagnostic.code << ": " << diagnostic.message;
        if (!diagnostic.path.empty()) {
            out << " (" << diagnostic.path << ")";
        }
        out << '\n';
    }
}

std::filesystem::path scene_asset_root(const std::filesystem::path& scene_path) {
    const auto scene_dir = scene_path.parent_path();
    if (scene_dir.empty()) return scene_dir;
    const auto folder_name = scene_dir.filename().string();
    if ((folder_name == "scenes" || folder_name == "project") && !scene_dir.parent_path().empty()) {
        return scene_dir.parent_path();
    }
    return scene_dir;
}

bool load_scene_context(const std::filesystem::path& scene_path, SceneSpec& scene, AssetResolutionTable& assets, std::ostream& err) {
    const auto parsed = parse_scene_spec_file(scene_path);
    if (!parsed.value.has_value()) {
        print_diagnostics(parsed.diagnostics, err);
        return false;
    }
    const auto validation = validate_scene_spec(*parsed.value);
    if (!validation.ok()) {
        print_diagnostics(validation.diagnostics, err);
        return false;
    }
    const auto resolved = resolve_assets(*parsed.value, scene_asset_root(scene_path));
    if (!resolved.value.has_value()) {
        print_diagnostics(resolved.diagnostics, err);
        return false;
    }
    scene = *parsed.value;
    assets = *resolved.value;
    return true;
}

} // namespace tachyon
