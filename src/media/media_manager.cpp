#include "tachyon/media/media_manager.h"
#include "tachyon/media/mesh_loader.h"

namespace tachyon::media {

const renderer2d::SurfaceRGBA* MediaManager::get_image(
    const std::filesystem::path& path, 
    AlphaMode alpha_mode,
    DiagnosticBag* diagnostics) {
    return m_image_manager.get_image(path, alpha_mode, diagnostics);
}

const HDRTextureData* MediaManager::get_hdr_image(
    const std::filesystem::path& path,
    DiagnosticBag* diagnostics) {
    return m_image_manager.get_hdr_image(path, diagnostics);
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
