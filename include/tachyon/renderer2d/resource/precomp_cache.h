#pragma once

#include <cstddef>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

#include "tachyon/renderer2d/core/framebuffer.h"

namespace tachyon::renderer2d {

class PrecompCache {
public:
    PrecompCache(std::size_t max_bytes = 1024 * 1024 * 512); // 512MB default
    
    std::shared_ptr<const SurfaceRGBA> get(const std::string& key) const;
    void put(const std::string& key, std::shared_ptr<SurfaceRGBA> surface);
    void clear();
    
    // Compatibility for tests
    std::shared_ptr<const SurfaceRGBA> lookup(const std::string& key) const { return get(key); }
    void store(const std::string& key, std::shared_ptr<SurfaceRGBA> surface) { put(key, std::move(surface)); }
    std::size_t entry_count() const { return cache_.size(); }
    
    std::size_t max_bytes() const { return max_bytes_; }
    void set_max_bytes(std::size_t bytes) { max_bytes_ = bytes; }
    
private:
    struct Entry {
        std::shared_ptr<SurfaceRGBA> surface;
        std::size_t size{0};
        std::list<std::string>::iterator lru_it;
    };

    mutable std::mutex mutex_;
    std::size_t max_bytes_;
    std::size_t current_bytes_{0};
    mutable std::list<std::string> lru_;
    std::unordered_map<std::string, Entry> cache_;
};

} // namespace tachyon::renderer2d
