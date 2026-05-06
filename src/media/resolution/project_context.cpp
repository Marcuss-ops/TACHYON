#include "tachyon/media/resolution/project_context.h"
#include "tachyon/media/resolution/asset_path_utils.h"

namespace tachyon::media {

ProjectResolutionContext ProjectResolutionContext::from_scene(const std::filesystem::path& scene_path) {
    ProjectResolutionContext context;
    
    if (scene_path.empty()) {
        context.project_root = std::filesystem::current_path();
    } else {
        context.project_root = scene_path.parent_path();
        if (context.project_root.empty()) {
            context.project_root = ".";
        }
    }

    context.assets_root = scene_asset_root(scene_path);
    if (context.assets_root.empty()) {
        context.assets_root = context.project_root;
    }

    context.fonts_root = context.assets_root / "fonts";
    context.sfx_root = context.assets_root / "audio";

    return context;
}

} // namespace tachyon::media
