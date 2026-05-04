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

std::optional<std::filesystem::path> AssetResolver::resolve_path(const std::string& spec, AssetType type) const {
    if (spec.empty()) return std::nullopt;

    std::filesystem::path p(spec);
    
    // 1. If absolute, use as is
    if (p.is_absolute()) {
        if (std::filesystem::exists(p)) return p;
        return std::nullopt;
    }

    // 2. Check AssetManager for registered IDs
    if (m_asset_manager) {
        if (auto asset = m_asset_manager->get_asset(spec)) {
            std::filesystem::path ap(asset->path);
            if (std::filesystem::exists(ap)) return ap;
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

    return std::nullopt;
}

ResolutionResult<std::filesystem::path> AssetResolver::resolve_path_strict(const std::string& spec, AssetType type, ResolveMode mode) const {
    ResolutionResult<std::filesystem::path> result;
    auto path = resolve_path(spec, type);
    
    if (path.has_value()) {
        result.value = path;
    } else {
        std::string msg = "Failed to resolve asset: " + spec;
        if (mode == ResolveMode::Strict) {
            result.diagnostics.add_error("ASSET_NOT_FOUND", msg);
        } else if (mode == ResolveMode::PermissiveWithWarning) {
            result.diagnostics.add_warning("ASSET_NOT_FOUND", msg);
        } else {
            result.diagnostics.add_info("ASSET_NOT_FOUND", msg);
        }
    }
    
    return result;
}

const renderer2d::SurfaceRGBA* AssetResolver::resolve_image(const std::string& spec, AlphaMode alpha_mode, ResolveMode mode) {
    auto shared = resolve_image_shared(spec, alpha_mode, mode);
    return shared.get();
}

std::shared_ptr<const renderer2d::SurfaceRGBA> AssetResolver::resolve_image_shared(const std::string& spec, AlphaMode alpha_mode, ResolveMode mode) {
    if (!m_image_manager) return nullptr;

    auto res = resolve_path_strict(spec, AssetType::IMAGE, mode);
    if (!res.value.has_value()) {
        return nullptr;
    }
    
    return m_image_manager->get_image_shared(*res.value, alpha_mode);
}

const text::Font* AssetResolver::resolve_font(const std::string& spec, std::uint32_t pixel_size, ResolveMode mode) {
    if (!m_font_registry) return nullptr;

    // First try if it's already loaded in the registry by name
    if (const auto* existing = m_font_registry->find(spec)) {
        return existing;
    }

    // If not, try to resolve path and load it
    auto res = resolve_path_strict(spec, AssetType::FONT, mode);
    if (res.value.has_value()) {
        std::filesystem::path path = *res.value;
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

    // Fallback logic
    if (mode == ResolveMode::Strict) {
        return nullptr;
    }
    
    return m_font_registry->default_font();
}

} // namespace tachyon::media
