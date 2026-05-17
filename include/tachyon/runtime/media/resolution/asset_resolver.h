#pragma once

#include "tachyon/runtime/media/management/image_manager.h"
#ifdef TACHYON_ENABLE_TEXT
#include "tachyon/text/fonts/core/font_registry.h"
#endif
#include "tachyon/core/assets/asset_resolution.h"

#include <filesystem>
#include <string>
#include <memory>
#include <optional>

#include "tachyon/core/media/asset_resolver_interface.h"

namespace tachyon::media {

class AssetManager;

/**
 * @brief Orchestrates asset path resolution and runtime loading.
 * 
 * AssetResolver is the single source of truth for mapping asset identifiers 
 * (paths, IDs, names) to physical files and runtime objects. It resolves the
 * hardcoded path issues and provides a unified interface for the renderer.
 */
class AssetResolver : public IAssetResolver {
public:

    AssetResolver(Config config,
                  AssetManager* asset_manager = nullptr,
                  ImageManager* image_manager = nullptr,
#ifdef TACHYON_ENABLE_TEXT
                  text::FontRegistry* font_registry = nullptr
#else
                  void* font_registry = nullptr
#endif
    );

    /**
     * @brief Resolves a string specification into an absolute filesystem path.
     * The spec can be an absolute path, a path relative to project_root/assets_root,
     * or a known asset ID in the AssetManager.
     */
    std::optional<std::filesystem::path> resolve_path(const std::string& spec, AssetType type = AssetType::IMAGE) const override;

    /**
     * @brief Resolves a path with detailed diagnostic reporting.
     */
    ResolutionResult<std::filesystem::path> resolve_path_strict(const std::string& spec, AssetType type, ::tachyon::ResolveMode mode) const;

    /**
     * @brief Resolves and loads an image surface.
     */
    const renderer2d::SurfaceRGBA* resolve_image(const std::string& spec, ::tachyon::AlphaMode alpha_mode = ::tachyon::AlphaMode::Straight, ::tachyon::ResolveMode mode = ::tachyon::ResolveMode::PermissiveWithWarning);

    /**
     * @brief Resolves and loads an image surface, returning a shared pointer for safe ownership.
     */
    std::shared_ptr<const renderer2d::SurfaceRGBA> resolve_image_shared(const std::string& spec, ::tachyon::AlphaMode alpha_mode = ::tachyon::AlphaMode::Straight, ::tachyon::ResolveMode mode = ::tachyon::ResolveMode::PermissiveWithWarning) override;

    /**
     * @brief Resolves all assets in a SceneSpec.
     */
    core::assets::AssetResolutionTable resolve_all(const SceneSpec& scene) const;

    /**
     * @brief Resolves and loads a font.
     */
#ifdef TACHYON_ENABLE_TEXT
    const text::Font* resolve_font(const std::string& spec, std::uint32_t pixel_size = 48, ::tachyon::ResolveMode mode = ::tachyon::ResolveMode::PermissiveWithWarning);
#endif

    /**
     * @brief Direct access to the underlying managers.
     */
    AssetManager* asset_manager() const { return m_asset_manager; }
    ImageManager* image_manager() const { return m_image_manager; }
#ifdef TACHYON_ENABLE_TEXT
    text::FontRegistry* font_registry() const { return m_font_registry; }
#endif

    const Config& config() const override { return m_config; }

private:
    Config m_config;
    AssetManager* m_asset_manager;
    ImageManager* m_image_manager;
#ifdef TACHYON_ENABLE_TEXT
    text::FontRegistry* m_font_registry;
#else
    void* m_font_registry;
#endif
};

} // namespace tachyon::media
