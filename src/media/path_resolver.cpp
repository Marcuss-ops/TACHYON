#include "tachyon/media/path_resolver.h"
#include "tachyon/core/io/path_resolution.h"
#include <filesystem>
#include <system_error>

namespace tachyon::media {

PathResolver::PathResolver(PathResolveContext context) 
    : context_(std::move(context)) {}

void PathResolver::set_context(PathResolveContext context) {
    context_ = std::move(context);
}

PathResolveResult PathResolver::resolve(const PathResolveRequest& request) const {
    PathResolveResult result;
    
    // Step 1: Resolve URI to absolute original path
    std::filesystem::path input_path(request.uri);
    
    if (!input_path.is_absolute()) {
        // Try relative to scene file first
        if (!context_.scene_file.empty()) {
            auto scene_dir = context_.scene_file.parent_path();
            auto relative_to_scene = scene_dir / input_path;
            if (std::filesystem::exists(relative_to_scene)) {
                result.original_path = std::filesystem::weakly_canonical(relative_to_scene);
                result.exists = true;
            }
        }
        
        // Then try relative to project root
        if (!result.exists && !context_.project_root.empty()) {
            auto relative_to_root = context_.project_root / input_path;
            if (std::filesystem::exists(relative_to_root)) {
                result.original_path = std::filesystem::weakly_canonical(relative_to_root);
                result.exists = true;
            }
        }
        
        // Then try relative to asset root
        if (!result.exists && !context_.asset_root.empty()) {
            auto relative_to_asset = context_.asset_root / input_path;
            if (std::filesystem::exists(relative_to_asset)) {
                result.original_path = std::filesystem::weakly_canonical(relative_to_asset);
                result.exists = true;
            }
        }
    } else {
        result.original_path = input_path;
        result.exists = std::filesystem::exists(input_path);
    }
    
    // Step 2: Determine runtime path (original or proxy)
    result.runtime_path = result.original_path;
    
    if (context_.prefer_proxy && result.exists) {
        // Use ProxyManifest to find proxy if available
        auto proxy_path = proxy_manifest_.find_proxy(
            result.original_path.string(), 
            ProxyProfile::Playback
        );
        if (!proxy_path.empty() && std::filesystem::exists(proxy_path)) {
            result.runtime_path = proxy_path;
            result.uses_proxy = true;
        }
    }
    
    // Step 3: Set reason if missing
    if (!result.exists && !request.allow_missing) {
        result.reason = "Asset not found: " + request.uri;
    }
    
    return result;
}

} // namespace tachyon::media
