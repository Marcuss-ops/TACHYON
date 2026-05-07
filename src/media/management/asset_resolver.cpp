#include "tachyon/media/management/asset_resolver.h"
#include "tachyon/media/resolution/asset_collector.h"
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

    // 5. Try with common extensions if no extension
    if (p.extension().empty()) {
        std::vector<std::string> extensions;
        if (type == AssetType::IMAGE) extensions = {".png", ".jpg", ".jpeg", ".webp"};
        else if (type == AssetType::AUDIO) extensions = {".mp3", ".wav", ".aac", ".m4a"};
        else if (type == AssetType::FONT) extensions = {".ttf", ".otf"};

        for (const auto& ext : extensions) {
            auto path_with_ext = resolve_path(spec + ext, type);
            if (path_with_ext) return path_with_ext;
        }
    }

    return std::nullopt;
}

ResolutionResult<std::filesystem::path> AssetResolver::resolve_path_strict(const std::string& spec, AssetType type, ResolveMode mode) const {
    ResolutionResult<std::filesystem::path> result;
    auto path = resolve_path(spec, type);
    
    if (path.has_value()) {
        result.value = path;
    } else {
        std::string type_name = (type == AssetType::AUDIO) ? "AUDIO" : (type == AssetType::IMAGE ? "IMAGE" : "FONT");
        std::string msg = "Failed to resolve " + type_name + " asset: '" + spec + "'. Check roots and path existence.";
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

AssetResolutionTable AssetResolver::resolve_all(const SceneSpec& scene) const {
    AssetResolutionTable table;
    auto refs = collect_asset_references(scene);
    
    for (const auto& ref : refs) {
        // Only interested in assets with IDs for the table
        if (ref.id.empty()) continue;

        ResolvedAsset resolved;
        resolved.asset_id = ref.id;
        const std::string tag = (ref.kind == media::AssetKind::Audio) ? "audio" : 
                        (ref.kind == media::AssetKind::Video) ? "video" : "image";
        resolved.type = tag;

        media::AssetType media_type = media::AssetType::IMAGE;
        if (ref.kind == media::AssetKind::Audio) media_type = media::AssetType::AUDIO;
        else if (ref.kind == media::AssetKind::Font) media_type = media::AssetType::FONT;
        else if (ref.kind == media::AssetKind::Video) media_type = media::AssetType::VIDEO;

        auto path = resolve_path(ref.source, media_type);
        if (path) {
            resolved.absolute_path = *path;
            resolved.exists = true;
        } else {
            resolved.absolute_path = ref.source;
            resolved.exists = false;
        }
        table[ref.id] = std::move(resolved);
    }
    return table;
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
