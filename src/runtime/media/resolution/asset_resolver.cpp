#include "tachyon/runtime/media/resolution/asset_resolver.h"
#include "tachyon/runtime/media/management/asset_manager.h"
#include "tachyon/core/assets/asset_collector.h"
#include "tachyon/core/media/resolved_asset.h"
#include "tachyon/runtime/resource/render_context.h"
#ifdef TACHYON_ENABLE_MEDIA
#include "tachyon/runtime/media/management/media_manager.h"
#endif
#include <iostream>
#include <algorithm>
#include <string>
#include <map>

namespace tachyon::media {

ResolvedAsset AssetResolver::resolve(const AssetRequest& request, RenderContext& context) const {
    ResolvedAsset result;
    result.id = request.id;
    result.kind = request.kind;

    AssetType type = AssetType::UNKNOWN;
    switch (request.kind) {
        case AssetKind::Image: type = AssetType::IMAGE; result.type_name = "image"; break;
        case AssetKind::Video: type = AssetType::VIDEO; result.type_name = "video"; break;
        case AssetKind::Audio: type = AssetType::AUDIO; result.type_name = "audio"; break;
        case AssetKind::Font: type = AssetType::FONT; result.type_name = "font"; break;
        default: break;
    }
    result.type = type;

    // 1. Resolve Path
    std::string path_spec = request.path.string();
    if (path_spec.empty()) {
        path_spec = request.id;
    }

    auto path_opt = this->resolve_path(path_spec, type);
    if (!path_opt) {
        result.exists = false;
        result.source_path = path_spec;
        result.diagnostics.add_error("ASSET_NOT_FOUND", "Failed to resolve asset path: " + path_spec);
        return result;
    }
    
    result.source_path = *path_opt;
    result.runtime_path = *path_opt;
    result.exists = true;

#ifdef TACHYON_ENABLE_MEDIA
    auto media_mgr = std::dynamic_pointer_cast<MediaManager>(context.media);
    
    // 2. Proxy Fallback (if applicable)
    if (media_mgr && request.purpose == ResolutionPurpose::Playback && media_mgr->use_proxies()) {
        if (type == AssetType::VIDEO || type == AssetType::IMAGE) {
            std::string proxy_path = media_mgr->proxy_manifest().resolve_for_playback(result.source_path.string(), request.target_width ? request.target_width : 1280);
            if (proxy_path != result.source_path.string()) {
                result.runtime_path = proxy_path;
                result.uses_proxy = true;
            }
        }
    }

    // 3. Decode & Load Data
    if (type == AssetType::IMAGE) {
        if (media_mgr) {
            const renderer2d::SurfaceRGBA* img = media_mgr->get_image(result, request.alpha_mode, &result.diagnostics);
            if (img) {
                // Return a lightweight view or shared copy if it's cached.
                // MediaManager usually caches it. But we need a shared_ptr for the API.
                // Actually, our ImageManager provides shared_ptr.
                result.image_frame = media_mgr->image_manager().get_image_shared(result.runtime_path, request.alpha_mode);
            }
        } else if (m_image_manager) {
            result.image_frame = m_image_manager->get_image_shared(result.runtime_path, request.alpha_mode);
        }
    } else if (type == AssetType::VIDEO) {
        if (media_mgr) {
            const renderer2d::SurfaceRGBA* frame = media_mgr->get_video_frame(result, request.time_seconds, &result.diagnostics);
            if (frame) {
                // The frame is managed by MediaManager's cache. 
                // We'd ideally return a shared_ptr, but for now we create a lightweight aliasing pointer or similar.
                // To keep it safe, since MediaManager returns raw ptr, we might not have a shared_ptr natively.
                // However, AssetResolver doesn't own MediaManager cache. We will leave image_frame null or wrap it dangerously if strictly needed,
                // but usually Video layers use the MediaManager directly if image_frame is null.
                // Better approach: make MediaManager return shared_ptr or let layers fetch it.
                // For now, if we have a pool, we just wrap it with a no-op deleter for the API.
                result.image_frame = std::shared_ptr<const renderer2d::SurfaceRGBA>(frame, [](const renderer2d::SurfaceRGBA*){});
            }
        }
    }
#endif

    return result;
}

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
