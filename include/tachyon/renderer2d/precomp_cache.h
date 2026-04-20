#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

#include "tachyon/renderer2d/framebuffer.h"

namespace tachyon::renderer2d {

class PrecompCache {
public:
    PrecompCache(std::size_t max_bytes = 1024 * 1024 * 512); // 512MB default
    
    std::shared_ptr<SurfaceRGBA> get(const std::string& key) const;
    void put(const std::string& key, std::shared_ptr<SurfaceRGBA> surface);
    void clear();
    
    // Compatibility for tests
    std::shared_ptr<SurfaceRGBA> lookup(const std::string& key) const { return get(key); }
    void store(const std::string& key, std::shared_ptr<SurfaceRGBA> surface) { put(key, std::move(surface)); }
    std::size_t entry_count() const { return cache_.size(); }
    
    std::size_t max_bytes() const { return max_bytes_; }
    void set_max_bytes(std::size_t bytes) { max_bytes_ = bytes; }
    
private:
    std::size_t max_bytes_;
    std::size_t current_bytes_{0};
    std::unordered_map<std::string, std::shared_ptr<SurfaceRGBA>> cache_;
};

} // namespace tachyon::renderer2d
