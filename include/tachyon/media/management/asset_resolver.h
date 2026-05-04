#pragma once

#include "tachyon/media/asset_manager.h"
#include "tachyon/media/management/image_manager.h"
#include "tachyon/text/fonts/core/font_registry.h"
#include <filesystem>
#include <string>
#include <memory>

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
    std::filesystem::path resolve_path(const std::string& spec, AssetType type = AssetType::IMAGE) const;

    /**
     * @brief Resolves and loads an image surface.
     */
    const renderer2d::SurfaceRGBA* resolve_image(const std::string& spec, AlphaMode alpha_mode = AlphaMode::Straight);

    /**
     * @brief Resolves and loads a font.
     */
    const text::Font* resolve_font(const std::string& spec, std::uint32_t pixel_size = 48);

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
