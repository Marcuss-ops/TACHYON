#pragma once

#include "tachyon/media/management/media_asset.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace tachyon::media {

class ProxyManager {
public:
    ProxyManager() = default;
    ~ProxyManager() = default;

    /**
     * @brief Queues a background transcode task for an asset.
     */
    void request_proxy_generation(std::shared_ptr<MediaAsset> asset) {
        if (asset) {
            asset->set_state(MediaAssetState::ProxyPending);
            // In a real implementation, this would queue to a background thread pool
            // using FFmpeg or similar to generate a low-res intra-frame proxy.
        }
    }

    /**
     * @brief Checks status of background tasks.
     */
    void update() {
        // Poll background jobs and update asset states to ProxyAvailable
    }
};

class MediaCache {
public:
    MediaCache(std::size_t max_memory_bytes) 
        : m_max_memory(max_memory_bytes) {}

    /**
     * @brief Caches metadata for an asset to avoid re-probing the file.
     */
    void cache_metadata(const std::string& asset_id, const std::string& metadata) {
        m_metadata_cache[asset_id] = metadata;
    }

    bool has_metadata(const std::string& asset_id) const {
        return m_metadata_cache.find(asset_id) != m_metadata_cache.end();
    }

    std::string get_metadata(const std::string& asset_id) const {
        auto it = m_metadata_cache.find(asset_id);
        if (it != m_metadata_cache.end()) return it->second;
        return "";
    }

    // A real implementation would also have an LRU cache for decoded frames (SurfaceRGBA)
    // std::shared_ptr<SurfaceRGBA> get_frame(const std::string& asset_id, double time);
    // void put_frame(const std::string& asset_id, double time, std::shared_ptr<SurfaceRGBA> frame);

private:
    std::size_t m_max_memory;
    std::unordered_map<std::string, std::string> m_metadata_cache;
};

} // namespace tachyon::media
