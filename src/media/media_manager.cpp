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
    return m_image_manager.get_image(path, alpha_mode, diagnostics);
}

const HDRTextureData* MediaManager::get_hdr_image(
    const std::filesystem::path& path,
    DiagnosticBag* diagnostics) {
    (void)diagnostics;
    return m_image_manager.get_hdr_image(path, diagnostics);
}

const renderer2d::SurfaceRGBA* MediaManager::get_video_frame(const std::filesystem::path& path, double time, DiagnosticBag* diagnostics) {
    (void)diagnostics;
    // 1. Check cache first (Fast path - no I/O)
    std::string key = path.string() + "@" + std::to_string(time);
    const auto* cached = m_image_manager.get_image(key, AlphaMode::Straight, nullptr);
    if (cached) return cached;

    // 2. Resolve path and handle offline/loading states
    if (m_fallback_policy == MediaFallbackPolicy::ReturnOffline) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_offline_placeholder) {
            m_offline_placeholder = std::make_shared<renderer2d::SurfaceRGBA>(1920, 1080);
            m_offline_placeholder->clear({0.1f, 0.1f, 0.1f, 1.0f}); // Dark gray Loading/Offline state
        }
        return m_offline_placeholder.get();
    }

    return nullptr;
}

void MediaManager::register_asset(std::shared_ptr<MediaAsset> asset) {
    if (!asset) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_assets[asset->descriptor().original_path] = asset;
}

std::filesystem::path MediaManager::resolve_media_path(const std::filesystem::path& path) const {
    if (m_use_proxies) {
        // Try global manifest resolution
        std::string resolved = m_proxy_manifest.resolve_for_playback(path.string(), 1280);
        if (resolved != path.string()) {
            return resolved;
        }
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_assets.find(path.string());
    if (it == m_assets.end()) return path;

    const auto& asset = it->second;
    if (m_use_proxies && asset->state() == MediaAssetState::ProxyAvailable) {
        return asset->descriptor().proxy_path;
    }
    
    if (asset->state() == MediaAssetState::Offline) {
        return "";
    }

    return asset->descriptor().original_path;
}

VideoDecoder* MediaManager::acquire_video_decoder(const std::filesystem::path& path) {
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

    std::lock_guard<std::mutex> pool_lock(pool->mutex);
    if (!pool->available.empty()) {
        auto decoder = std::move(pool->available.back());
        pool->available.pop_back();
        return decoder.release();
    }

    auto decoder = std::make_unique<VideoDecoder>();
    if (!decoder->open(path)) {
        return nullptr;
    }
    return decoder.release();
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
    // We use a specific key format for video frames: "path@time"
    std::string key = path + "@" + std::to_string(time);
    m_image_manager.store_image(key, std::move(frame));
}

void MediaManager::clear_cache() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_image_manager.clear_cache();
    m_video_pools.clear();
    m_mesh_cache.clear();
}

DiagnosticBag MediaManager::consume_diagnostics() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_image_manager.consume_diagnostics();
}

} // namespace tachyon::media
