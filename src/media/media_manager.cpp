#include "tachyon/media/management/media_manager.h"
#include "tachyon/media/loading/mesh_loader.h"

namespace tachyon::media {

MediaManager::MediaManager()
    : m_proxy_worker(std::make_unique<ProxyWorker>(m_proxy_manifest)) {}

MediaManager::~MediaManager() = default;

const renderer2d::SurfaceRGBA* MediaManager::get_image(
    const std::filesystem::path& path, 
    AlphaMode alpha_mode,
    DiagnosticBag* diagnostics) {
    (void)diagnostics;
    
    std::filesystem::path resolved = resolve_media_path(path, ResolutionPurpose::Playback);
    return m_image_manager.get_image(resolved, alpha_mode, diagnostics);
}

const HDRTextureData* MediaManager::get_hdr_image(
    const std::filesystem::path& path,
    DiagnosticBag* diagnostics) {
    (void)diagnostics;
    
    std::filesystem::path resolved = resolve_media_path(path, ResolutionPurpose::Playback);
    return m_image_manager.get_hdr_image(resolved, diagnostics);
}

const renderer2d::SurfaceRGBA* MediaManager::get_video_frame(const std::filesystem::path& path, double time, DiagnosticBag* diagnostics) {
    std::filesystem::path resolved = resolve_media_path(path, ResolutionPurpose::Playback);
    const std::string key = resolved.string() + "@" + std::to_string(time);

    if (m_frame_cache) {
        auto cached = m_frame_cache->get(key);
        if (cached) return cached;
    }

    if (m_fallback_policy == MediaFallbackPolicy::ReturnOffline && resolved.empty()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_offline_placeholder || 
            m_offline_placeholder->width() != m_placeholder_width || 
            m_offline_placeholder->height() != m_placeholder_height) {
            m_offline_placeholder = std::make_shared<renderer2d::SurfaceRGBA>(m_placeholder_width, m_placeholder_height);
            m_offline_placeholder->clear({0.1f, 0.1f, 0.1f, 1.0f});
        }
        return m_offline_placeholder.get();
    }

    auto decoder = acquire_video_decoder(resolved);
    if (!decoder) {
        if (diagnostics) diagnostics->add_error("media.video.decode_failed", "failed to acquire video decoder", resolved.string());
        return nullptr;
    }

    std::optional<renderer2d::SurfaceRGBA> decoded = decoder->get_frame_at_time(time);
    if (!decoded.has_value()) {
        if (diagnostics) diagnostics->add_error("media.video.decode_failed", "failed to decode video frame", resolved.string());
        return nullptr;
    }

    auto frame = std::make_unique<renderer2d::SurfaceRGBA>(*decoded);
    const renderer2d::SurfaceRGBA* ptr = frame.get();
    if (m_frame_cache) {
        m_frame_cache->put(key, std::move(frame));
    }
    return ptr;
}

void MediaManager::register_asset(std::shared_ptr<MediaAsset> asset) {
    if (!asset) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_assets[asset->id()] = asset;
}

std::filesystem::path MediaManager::get_asset_path(const std::string& asset_id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_assets.find(asset_id);
    if (it != m_assets.end()) {
        return it->second->descriptor().original_path;
    }
    return {};
}

std::filesystem::path MediaManager::resolve_media_path(
    const std::filesystem::path& path,
    ResolutionPurpose purpose,
    uint32_t target_width) const {
    
    const bool allow_proxy = (purpose == ResolutionPurpose::Playback) && m_use_proxies;

    if (allow_proxy) {
        // Default proxy resolution if target_width is 0
        uint32_t resolve_width = target_width > 0 ? target_width : 1280;
        
        // Try global manifest resolution
        std::string resolved = m_proxy_manifest.resolve_for_playback(path.string(), resolve_width);
        
        // 8K Proxy Fallback Logic:
        // If we are looking for a proxy and the original is very high res (e.g. 8K),
        // but the requested proxy isn't available, try intermediate resolutions.
        if (resolved == path.string()) {
            // If target was low-res (e.g. 1280), try 2K and 4K as fallbacks
            if (resolve_width <= 1280) {
                resolved = m_proxy_manifest.resolve_for_playback(path.string(), 1920); // Try 2K
                if (resolved == path.string()) {
                    resolved = m_proxy_manifest.resolve_for_playback(path.string(), 3840); // Try 4K
                }
            } else if (resolve_width <= 1920) {
                resolved = m_proxy_manifest.resolve_for_playback(path.string(), 3840); // Try 4K
            }
        }

        if (resolved != path.string()) {
            return resolved;
        }
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_assets.find(path.string());
    if (it == m_assets.end()) return path;

    const auto& asset = it->second;
    if (allow_proxy && asset->state() == MediaAssetState::ProxyAvailable) {
        return asset->descriptor().proxy_path;
    }
    
    if (asset->state() == MediaAssetState::Offline) {
        if (m_fallback_policy == MediaFallbackPolicy::ReturnOffline) {
             return ""; // Will trigger placeholder
        }
        return path; // Try anyway
    }

    return asset->descriptor().original_path;
}

MediaManager::PooledVideoDecoder MediaManager::acquire_video_decoder(const std::filesystem::path& path) {
    const std::string key = path.string();
    std::shared_ptr<VideoPool> pool;
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_video_pools.find(key);
        if (it == m_video_pools.end()) {
            pool = std::make_shared<VideoPool>();
            m_video_pools.emplace(key, pool);
        } else {
            pool = it->second;
        }
    }

    {
        std::lock_guard<std::mutex> pool_lock(pool->mutex);
        if (!pool->available.empty()) {
            auto decoder = std::move(pool->available.back());
            pool->available.pop_back();
            
            // Return RAII wrapper with custom deleter that returns to pool
            return PooledVideoDecoder(decoder.release(), [this, path](VideoDecoder* d) {
                this->release_video_decoder(path, d);
            });
        }
    }

    auto decoder = std::make_unique<VideoDecoder>();
    if (!decoder->open(path)) {
        return nullptr;
    }
    
    return PooledVideoDecoder(decoder.release(), [this, path](VideoDecoder* d) {
        this->release_video_decoder(path, d);
    });
}

void MediaManager::release_video_decoder(const std::filesystem::path& path, VideoDecoder* decoder) {
    if (!decoder) return;
    
    const std::string key = path.string();
    std::shared_ptr<VideoPool> pool;
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_video_pools.find(key);
        if (it == m_video_pools.end()) {
            // Pool should exist if we acquired a decoder, but handle cleanup if not
            delete decoder;
            return;
        }
        pool = it->second;
    }

    std::lock_guard<std::mutex> pool_lock(pool->mutex);
    pool->available.emplace_back(std::unique_ptr<VideoDecoder>(decoder));
}

const MeshAsset* MediaManager::get_mesh(const std::filesystem::path& path, DiagnosticBag* diagnostics) {
    const std::string key = path.string();
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_mesh_cache.find(key);
    if (it != m_mesh_cache.end()) {
        return it->second.get();
    }

    auto mesh = MeshLoader::load_from_gltf(path, diagnostics);
    if (!mesh) return nullptr;

    const MeshAsset* ptr = mesh.get();
    m_mesh_cache[key] = std::move(mesh);
    return ptr;
}

void MediaManager::store_video_frame(const std::string& path, double time, std::unique_ptr<renderer2d::SurfaceRGBA> frame) {
    std::string key = path + "@" + std::to_string(time);
    const renderer2d::SurfaceRGBA* ptr = frame.get();
    if (m_frame_cache) {
        m_frame_cache->put(key, std::move(frame));
    }
    if (ptr) {
        m_image_manager.store_image(key, std::make_unique<renderer2d::SurfaceRGBA>(*ptr));
    }
}

void MediaManager::clear_cache() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_image_manager.clear_cache();
    m_video_pools.clear();
    if (m_frame_cache) m_frame_cache->clear();
    m_mesh_cache.clear();
}

DiagnosticBag MediaManager::consume_diagnostics() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_image_manager.consume_diagnostics();
}

} // namespace tachyon::media
