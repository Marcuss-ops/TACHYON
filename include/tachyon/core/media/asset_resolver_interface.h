#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/core/render_types.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/core/media/media_types.h"
#include <string>
#include <memory>
#include <filesystem>
#include <optional>

namespace tachyon::media {

/**
 * @brief Interface for resolving asset paths and objects.
 */
class IAssetResolver {
public:
    virtual ~IAssetResolver() = default;

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
