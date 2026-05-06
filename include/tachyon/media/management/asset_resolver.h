#pragma once

#include "tachyon/media/asset_manager.h"
#include "tachyon/media/management/image_manager.h"
#include "tachyon/text/fonts/core/font_registry.h"
#include "tachyon/media/resolution/asset_resolution.h"
#include "tachyon/media/resolution/project_context.h"
#include <filesystem>
#include <string>
#include <memory>
#include <optional>

namespace tachyon::media {

/**
 * @brief Orchestrates asset path resolution and runtime loading.
 * 
 * AssetResolver is the single source of truth for mapping asset identifiers 
 * (paths, IDs, names) to physical files and runtime objects. It resolves the
 * hardcoded path issues and provides a unified interface for the renderer.
 */
class AssetResolver {
public:
    struct Config {
        std::filesystem::path project_root;    ///< Root of the current project/scene
        std::filesystem::path assets_root;     ///< Global assets directory
        std::filesystem::path sfx_root;        ///< Root for sound effects
        std::filesystem::path fonts_root;      ///< Root for font files

        Config() = default;
        Config(const ProjectResolutionContext& ctx)
            : project_root(ctx.project_root)
            , assets_root(ctx.assets_root)
            , sfx_root(ctx.sfx_root)
            , fonts_root(ctx.fonts_root) {}
    };

    AssetResolver(Config config,
                  AssetManager* asset_manager = nullptr,
                  ImageManager* image_manager = nullptr,
                  text::FontRegistry* font_registry = nullptr);

    /**
     * @brief Resolves a string specification into an absolute filesystem path.
     * The spec can be an absolute path, a path relative to project_root/assets_root,
     * or a known asset ID in the AssetManager.
     */
    std::optional<std::filesystem::path> resolve_path(const std::string& spec, AssetType type = AssetType::IMAGE) const;

    /**
     * @brief Resolves a path with detailed diagnostic reporting.
     */
    ResolutionResult<std::filesystem::path> resolve_path_strict(const std::string& spec, AssetType type, ResolveMode mode) const;

    /**
     * @brief Resolves and loads an image surface.
     */
    const renderer2d::SurfaceRGBA* resolve_image(const std::string& spec, AlphaMode alpha_mode = AlphaMode::Straight, ResolveMode mode = ResolveMode::PermissiveWithWarning);

    /**
     * @brief Resolves and loads an image surface, returning a shared pointer for safe ownership.
     */
    std::shared_ptr<const renderer2d::SurfaceRGBA> resolve_image_shared(const std::string& spec, AlphaMode alpha_mode = AlphaMode::Straight, ResolveMode mode = ResolveMode::PermissiveWithWarning);

    /**
     * @brief Resolves all assets in a SceneSpec.
     */
    AssetResolutionTable resolve_all(const SceneSpec& scene) const;

    /**
     * @brief Resolves and loads a font.
     */
    const text::Font* resolve_font(const std::string& spec, std::uint32_t pixel_size = 48, ResolveMode mode = ResolveMode::PermissiveWithWarning);

    /**
     * @brief Direct access to the underlying managers.
     */
    AssetManager* asset_manager() const { return m_asset_manager; }
    ImageManager* image_manager() const { return m_image_manager; }
    text::FontRegistry* font_registry() const { return m_font_registry; }

    const Config& config() const { return m_config; }

private:
    Config m_config;
    AssetManager* m_asset_manager;
    ImageManager* m_image_manager;
    text::FontRegistry* m_font_registry;
};

} // namespace tachyon::media
