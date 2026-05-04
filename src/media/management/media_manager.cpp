#include "tachyon/media/management/media_manager.h"
#include "tachyon/media/loading/mesh_loader.h"

namespace tachyon::media {

MediaManager::MediaManager()
    : m_proxy_worker(std::make_unique<ProxyWorker>(m_proxy_manifest)),
      m_path_resolver(PathResolveContext{}) {
    
    // Default budget: 2GB
    m_frame_cache = std::make_unique<ResourceCache<std::string, renderer2d::SurfaceRGBA>>(
        2ULL * 1024 * 1024 * 1024,
        [](const renderer2d::SurfaceRGBA& s) {
            return static_cast<std::size_t>(s.width()) * s.height() * 4 * sizeof(float);
        }
    );
}

MediaManager::~MediaManager() = default;

void MediaManager::set_memory_budget(std::size_t bytes) {
    m_frame_cache->set_budget(bytes);
}

std::size_t MediaManager::current_memory_usage() const {
    return m_frame_cache->current_usage();
}

const renderer2d::SurfaceRGBA* MediaManager::get_image(
    const std::filesystem::path& path, 
    AlphaMode alpha_mode,
    DiagnosticBag* diagnostics) {
    (void)diagnostics;
    
    std::filesystem::path resolved = resolve_media_path(path, ResolutionPurpose::Playback);
    
    // Check our central cache first
    const std::string key = resolved.string() + ":img:" + std::to_string(static_cast<int>(alpha_mode));
    if (auto cached = m_frame_cache->get(key)) {
        return cached;
    }

    // Fallback to ImageManager but store in our cache
    auto img = m_image_manager.get_image(resolved, alpha_mode, diagnostics);
    if (img) {
        // We need to copy because ImageManager owns its cache
        // TODO: Refactor ImageManager to use the same central cache
        auto copy = std::make_unique<renderer2d::SurfaceRGBA>(*img);
        const renderer2d::SurfaceRGBA* ptr = copy.get();
        m_frame_cache->put(key, std::move(copy));
        return ptr;
    }
    return nullptr;
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

    if (auto cached = m_frame_cache->get(key)) {
        return cached;
    }

    if (m_fallback_policy == MediaFallbackPolicy::ReturnOffline && resolved.empty()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_offline_placeholder) {
            m_offline_placeholder = std::make_shared<renderer2d::SurfaceRGBA>(1920, 1080);
            m_offline_placeholder->clear({0.1f, 0.1f, 0.1f, 1.0f});
        }
        return m_offline_placeholder.get();
    }

    PooledVideoDecoder decoder = acquire_video_decoder(resolved);
    if (!decoder) {
        if (diagnostics) {
            diagnostics->add_error("media.video.decode_failed", "failed to acquire video decoder", resolved.string());
        }
        return nullptr;
    }

    // Optimize: Check if video is 8K and enforce proxy if not in Export mode
    // (In this context we are usually in Playback)
    if (decoder->width() > 4096 && m_use_proxies) {
        // If we reached here, it means resolve_media_path didn't find a proxy
        // We should probably trigger proxy generation or at least warn
    }

    auto frame = std::make_unique<renderer2d::SurfaceRGBA>(1, 1);
    if (decoder->get_frame_into(time, *frame)) {
        const renderer2d::SurfaceRGBA* ptr = frame.get();
        m_frame_cache->put(key, std::move(frame));
        return ptr;
    }

    if (diagnostics) {
        diagnostics->add_error("media.video.decode_failed", "failed to decode video frame", resolved.string());
    }
    return nullptr;
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
    ResolutionPurpose purpose) const {
    
    // Use PathResolver for centralized path resolution
    PathResolveRequest request;
    request.uri = path.string();
    request.allow_missing = (purpose != ResolutionPurpose::Playback);
    
    auto result = m_path_resolver.resolve(request);
    if (!result.runtime_path.empty()) {
        return result.runtime_path;
    }
    
    // Deprecated fallback logic - to be removed after transition
    const bool allow_proxy = (purpose == ResolutionPurpose::Playback) && m_use_proxies;

    if (allow_proxy) {
        std::string resolved = m_proxy_manifest.resolve_for_playback(path.string(), 1280);
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
             return "";
        }
        return path;
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

    auto deleter = [this, path](VideoDecoder* d) {
        this->release_video_decoder(path, d);
    };

    std::lock_guard<std::mutex> pool_lock(pool->mutex);
    if (!pool->available.empty()) {
        auto decoder = std::move(pool->available.back());
        pool->available.pop_back();
        return PooledVideoDecoder(decoder.release(), deleter);
    }

    auto decoder = std::make_unique<VideoDecoder>();
    if (!decoder->open(path)) {
        return PooledVideoDecoder(nullptr, deleter);
    }
    return PooledVideoDecoder(decoder.release(), deleter);
}

void MediaManager::release_video_decoder(const std::filesystem::path& path, VideoDecoder* decoder) {
    if (!decoder) return;
    
    const std::string key = path.string();
    std::shared_ptr<VideoPool> pool;
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_video_pools.find(key);
        if (it == m_video_pools.end()) {
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
    m_frame_cache->put(key, std::move(frame));
}

void MediaManager::clear_cache() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_image_manager.clear_cache();
    m_video_pools.clear();
    m_frame_cache->clear();
    m_mesh_cache.clear();
}

DiagnosticBag MediaManager::consume_diagnostics() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_image_manager.consume_diagnostics();
}

const renderer2d::SurfaceRGBA* MediaManager::get_image(
    const ResolvedAsset& asset,
    AlphaMode alpha_mode,
    DiagnosticBag* diagnostics) {
    return get_image(asset.runtime_path, alpha_mode, diagnostics);
}

const renderer2d::SurfaceRGBA* MediaManager::get_video_frame(
    const ResolvedAsset& asset,
    double time,
    DiagnosticBag* diagnostics) {
    return get_video_frame(asset.runtime_path, time, diagnostics);
}

const HDRTextureData* MediaManager::get_hdr_image(
    const ResolvedAsset& asset,
    DiagnosticBag* diagnostics) {
    return get_hdr_image(asset.runtime_path, diagnostics);
}

const MeshAsset* MediaManager::get_mesh(
    const ResolvedAsset& asset,
    DiagnosticBag* diagnostics) {
    return get_mesh(asset.runtime_path, diagnostics);
}

} // namespace tachyon::media
