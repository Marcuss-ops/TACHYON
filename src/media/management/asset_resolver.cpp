#include "tachyon/media/management/asset_resolver.h"
#include <iostream>

namespace tachyon::media {

AssetResolver::AssetResolver(Config config,
                             AssetManager* asset_manager,
                             ImageManager* image_manager,
                             text::FontRegistry* font_registry)
    : m_config(std::move(config))
    , m_asset_manager(asset_manager)
    , m_image_manager(image_manager)
    , m_font_registry(font_registry) {
}

std::filesystem::path AssetResolver::resolve_path(const std::string& spec, AssetType type) const {
    if (spec.empty()) return {};

    std::filesystem::path p(spec);
    
    // 1. If absolute, use as is
    if (p.is_absolute()) {
        return p;
    }

    // 2. Check AssetManager for registered IDs
    if (m_asset_manager) {
        if (auto asset = m_asset_manager->get_asset(spec)) {
            return std::filesystem::path(asset->path);
        }
    }

    // 3. Resolve based on type and relative roots
    std::filesystem::path resolved;
    
    switch (type) {
        case AssetType::AUDIO:
            if (!m_config.sfx_root.empty()) {
                resolved = m_config.sfx_root / p;
                if (std::filesystem::exists(resolved)) return resolved;
            }
            break;
        case AssetType::FONT:
            if (!m_config.fonts_root.empty()) {
                resolved = m_config.fonts_root / p;
                if (std::filesystem::exists(resolved)) return resolved;
            }
            break;
        default:
            break;
    }

    // 4. Fallback: try project_root then assets_root
    if (!m_config.project_root.empty()) {
        resolved = m_config.project_root / p;
        if (std::filesystem::exists(resolved)) return resolved;
    }

    if (!m_config.assets_root.empty()) {
        resolved = m_config.assets_root / p;
        if (std::filesystem::exists(resolved)) return resolved;
    }

    return p; // Return original if not found
}

const renderer2d::SurfaceRGBA* AssetResolver::resolve_image(const std::string& spec, AlphaMode alpha_mode) {
    if (!m_image_manager) return nullptr;

    std::filesystem::path path = resolve_path(spec, AssetType::IMAGE);
    return m_image_manager->get_image(path, alpha_mode);
}

const text::Font* AssetResolver::resolve_font(const std::string& spec, std::uint32_t pixel_size) {
    if (!m_font_registry) return nullptr;

    // First try if it's already loaded in the registry by name
    if (const auto* existing = m_font_registry->find(spec)) {
        return existing;
    }

    // If not, try to resolve path and load it
    std::filesystem::path path = resolve_path(spec, AssetType::FONT);
    if (std::filesystem::exists(path)) {
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext == ".ttf" || ext == ".otf") {
            if (m_font_registry->load_ttf(spec, path, pixel_size)) {
                return m_font_registry->find(spec);
            }
        } else if (ext == ".bdf") {
            if (m_font_registry->load_bdf(spec, path)) {
                return m_font_registry->find(spec);
            }
        }
    }

    return m_font_registry->default_font();
}

} // namespace tachyon::media
