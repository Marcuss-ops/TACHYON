#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/core/render_types.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/core/media/media_types.h"
#include "tachyon/core/media/resolved_asset.h"
#include <string>
#include <memory>
#include <filesystem>
#include <optional>

namespace tachyon { struct RenderContext; }

namespace tachyon::media {

/**
 * @brief Interface for resolving asset paths and objects.
 */
class IAssetResolver {
public:
    struct Config {
        std::filesystem::path project_root;    ///< Root of the current project/scene
        std::filesystem::path assets_root;     ///< Global assets directory
        std::filesystem::path sfx_root;        ///< Root for sound effects
        std::filesystem::path fonts_root;      ///< Root for font files

        Config() = default;
    };

    virtual ~IAssetResolver() = default;

    virtual const Config& config() const = 0;

    /**
     * @brief Centralized resolution method for all asset requests.
     */
    virtual ResolvedAsset resolve(const AssetRequest& request, RenderContext& context) const = 0;

    /**
     * @brief Resolves a specification into an absolute filesystem path.
     */
    virtual std::optional<std::filesystem::path> resolve_path(
        const std::string& spec, 
        AssetType type = AssetType::IMAGE) const = 0;

    /**
     * @brief Resolves and loads an image surface.
     */
    virtual std::shared_ptr<const renderer2d::SurfaceRGBA> resolve_image_shared(
        const std::string& spec, 
        ::tachyon::AlphaMode alpha_mode = ::tachyon::AlphaMode::Straight,
        ::tachyon::ResolveMode mode = ::tachyon::ResolveMode::PermissiveWithWarning) = 0;
};

} // namespace tachyon::media
