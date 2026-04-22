#include "tachyon/renderer2d/resource/precomp_cache.h"

#include <algorithm>

namespace tachyon::renderer2d {

PrecompCache::PrecompCache(std::size_t max_bytes) : max_bytes_(max_bytes) {}

std::shared_ptr<const SurfaceRGBA> PrecompCache::get(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        lru_.splice(lru_.begin(), lru_, it->second.lru_it);
        return it->second.surface;
    }
    return nullptr;
}

void PrecompCache::put(const std::string& key, std::shared_ptr<SurfaceRGBA> surface) {
    if (!surface) return;
    std::lock_guard<std::mutex> lock(mutex_);
    const std::size_t size = static_cast<std::size_t>(surface->width()) * static_cast<std::size_t>(surface->height()) * 4U;
    if (size > max_bytes_) return;

    auto existing = cache_.find(key);
    if (existing != cache_.end()) {
        current_bytes_ -= existing->second.size;
        lru_.erase(existing->second.lru_it);
        cache_.erase(existing);
    }

    while (!cache_.empty() && current_bytes_ + size > max_bytes_) {
        const std::string& evict_key = lru_.back();
        auto evict_it = cache_.find(evict_key);
        if (evict_it != cache_.end()) {
            current_bytes_ -= evict_it->second.size;
            cache_.erase(evict_it);
        }
        lru_.pop_back();
    }

    lru_.push_front(key);
    cache_.emplace(key, Entry{std::move(surface), size, lru_.begin()});
    current_bytes_ += size;
}

void PrecompCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
    lru_.clear();
    current_bytes_ = 0;
}

} // namespace tachyon::renderer2d
