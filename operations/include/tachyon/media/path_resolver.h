#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include "tachyon/media/management/proxy_manifest.h"
#include "tachyon/media/asset_manager.h"

namespace tachyon::media {

/**
 * Context for path resolution, set once per project/scene.
 */
struct PathResolveContext {
    std::filesystem::path project_root;
    std::filesystem::path scene_file;
    std::filesystem::path asset_root;
    std::filesystem::path cache_root;
    bool prefer_proxy{true};
};

/**
 * Request to resolve a single asset/path.
 */
struct PathResolveRequest {
    std::string uri;
    AssetType type{AssetType::PROJECT};
    bool allow_missing{false};
};

/**
 * Result of path resolution.
 */
struct PathResolveResult {
    std::filesystem::path original_path;
    std::filesystem::path runtime_path;
    bool exists{false};
    bool uses_proxy{false};
    std::string reason;
};

/**
 * Single source of truth for converting asset references to filesystem paths.
 *
 * Rules:
 * - Only PathResolver may convert asset references into filesystem paths.
 * - ProxyManifest only maps original paths to proxy paths.
 * - Renderers must not resolve filesystem paths directly.
 */
class PathResolver {
public:
    explicit PathResolver(PathResolveContext context);

    /**
     * Resolves a path request to concrete filesystem paths.
     */
    PathResolveResult resolve(const PathResolveRequest& request) const;

    /**
     * Updates the resolution context (e.g., when scene file changes).
     */
    void set_context(PathResolveContext context);

    /**
     * Returns the current context.
     */
    const PathResolveContext& context() const { return context_; }

private:
    PathResolveContext context_;
    ProxyManifest proxy_manifest_;
};

} // namespace tachyon::media
