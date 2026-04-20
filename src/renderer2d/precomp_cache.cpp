#include "tachyon/renderer2d/precomp_cache.h"

namespace tachyon::renderer2d {

PrecompCache::PrecompCache(std::size_t max_bytes) : max_bytes_(max_bytes) {}

std::shared_ptr<SurfaceRGBA> PrecompCache::get(const std::string& key) const {
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        return it->second;
    }
    return nullptr;
}

void PrecompCache::put(const std::string& key, std::shared_ptr<SurfaceRGBA> surface) {
    if (!surface) return;
    std::size_t size = surface->width() * surface->height() * 4;
    if (size > max_bytes_) return;
    
    // Simplistic cache eviction for now
    if (current_bytes_ + size > max_bytes_) {
        clear();
    }
    
    cache_[key] = surface;
    current_bytes_ += size;
}

void PrecompCache::clear() {
    cache_.clear();
    current_bytes_ = 0;
}

} // namespace tachyon::renderer2d
