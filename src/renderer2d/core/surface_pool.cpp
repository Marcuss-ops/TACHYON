#include "tachyon/renderer2d/core/surface_pool.h"

namespace tachyon::renderer2d {

std::shared_ptr<SurfaceRGBA> SurfacePool::acquire(uint32_t width, uint32_t height) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    PoolEntry entry{width, height};
    auto it = m_pool.find(entry);
    
    SurfaceRGBA* raw_surface = nullptr;
    if (it != m_pool.end() && !it->second.empty()) {
        raw_surface = it->second.back().release();
        it->second.pop_back();
    } else {
        raw_surface = new SurfaceRGBA(width, height);
    }
    
    return std::shared_ptr<SurfaceRGBA>(raw_surface, SurfaceDeleter{shared_from_this()});
}

void SurfacePool::release(SurfaceRGBA* surface) {
    if (!surface) return;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    PoolEntry entry{surface->width(), surface->height()};
    auto& bucket = m_pool[entry];
    
    if (bucket.size() < m_capacity) {
        bucket.push_back(std::unique_ptr<SurfaceRGBA>(surface));
    } else {
        delete surface;
    }
}

void SurfacePool::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pool.clear();
}

} // namespace tachyon::renderer2d
