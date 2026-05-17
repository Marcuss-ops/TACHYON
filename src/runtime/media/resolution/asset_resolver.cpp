#include "tachyon/runtime/media/resolution/asset_resolver.h"
#include "tachyon/runtime/media/management/asset_manager.h"
#include "tachyon/core/assets/asset_collector.h"
#include "tachyon/core/media/resolved_asset.h"
#include <iostream>
#include <algorithm>
#include <string>
#include <map>

namespace tachyon::media {

AssetResolver::AssetResolver(Config config,
                             AssetManager* asset_manager,
                             ImageManager* image_manager,
#ifdef TACHYON_ENABLE_TEXT
                             text::FontRegistry* font_registry
#else
                             void* font_registry
#endif
                             )
    : m_config(std::move(config)),
      m_asset_manager(asset_manager),
      m_image_manager(image_manager),
#ifdef TACHYON_ENABLE_TEXT
      m_font_registry(font_registry)
#else
      m_font_registry(nullptr)
#endif
{
    (void)font_registry;
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
            std::filesystem::path ap(asset->uri);
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
            auto path_with_ext = this->resolve_path(spec + ext, type);
            if (path_with_ext.has_value()) return path_with_ext;
        }
    }

    return std::nullopt;
}

ResolutionResult<std::filesystem::path> AssetResolver::resolve_path_strict(const std::string& spec, AssetType type, ::tachyon::ResolveMode mode) const {
    ResolutionResult<std::filesystem::path> result;
    auto path = this->resolve_path(spec, type);
    
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

const renderer2d::SurfaceRGBA* AssetResolver::resolve_image(const std::string& spec, ::tachyon::AlphaMode alpha_mode, ::tachyon::ResolveMode mode) {
    auto shared = this->resolve_image_shared(spec, alpha_mode, mode);
    return shared.get();
}

std::shared_ptr<const renderer2d::SurfaceRGBA> AssetResolver::resolve_image_shared(const std::string& spec, ::tachyon::AlphaMode alpha_mode, ::tachyon::ResolveMode mode) {
    if (!m_image_manager) return nullptr;

    auto res = this->resolve_path_strict(spec, AssetType::IMAGE, mode);
    if (!res.value.has_value()) {
        return nullptr;
    }
    
    return m_image_manager->get_image_shared(res.value.value(), alpha_mode);
}

core::assets::AssetResolutionTable AssetResolver::resolve_all(const SceneSpec& scene) const {
    core::assets::AssetResolutionTable table;
    auto refs = core::assets::collect_asset_references(scene);
    
    for (const auto& ref : refs) {
        if (ref.id.empty()) continue;

        ::tachyon::media::ResolvedAsset resolved;
        resolved.id = ref.id;
        resolved.type_name = (ref.kind == ::tachyon::media::AssetKind::Audio) ? "audio" : 
                            (ref.kind == ::tachyon::media::AssetKind::Video) ? "video" : "image";

        ::tachyon::media::AssetType media_type = ::tachyon::media::AssetType::IMAGE;
        if (ref.kind == ::tachyon::media::AssetKind::Audio) media_type = ::tachyon::media::AssetType::AUDIO;
        else if (ref.kind == ::tachyon::media::AssetKind::Font) media_type = ::tachyon::media::AssetType::FONT;
        else if (ref.kind == ::tachyon::media::AssetKind::Video) media_type = ::tachyon::media::AssetType::VIDEO;

        auto path_opt = this->resolve_path(ref.source, media_type);
        if (path_opt.has_value()) {
            resolved.source_path = path_opt.value();
            resolved.runtime_path = path_opt.value();
            resolved.exists = true;
        } else {
            resolved.source_path = ref.source;
            resolved.runtime_path = ref.source;
            resolved.exists = false;
        }
        
        const std::string key = ref.id;
        table[key] = std::move(resolved);
    }
    return table;
}
#ifdef TACHYON_ENABLE_TEXT
const text::Font* AssetResolver::resolve_font(const std::string& spec, std::uint32_t pixel_size, ::tachyon::ResolveMode mode) {
    if (!m_font_registry) return nullptr;

    if (const auto* existing = m_font_registry->find(spec)) {
        return existing;
    }

    auto res = this->resolve_path_strict(spec, AssetType::FONT, mode);
    if (res.value.has_value()) {
        std::filesystem::path path = res.value.value();
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
#endif

} // namespace tachyon::media
