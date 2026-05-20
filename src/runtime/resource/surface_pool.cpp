#include "tachyon/runtime/resource/surface_pool.h"

namespace tachyon {

SurfacePool::SurfacePool(const runtime::SurfacePoolPolicy& policy)
    : m_policy(policy) {}

std::shared_ptr<renderer2d::SurfaceRGBA> SurfacePool::acquire(std::uint32_t width, std::uint32_t height) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    PoolKey key{width, height};
    auto it = m_pool.find(key);
    
    std::unique_ptr<renderer2d::SurfaceRGBA> surface;
    if (it != m_pool.end() && !it->second.empty()) {
        surface = std::move(it->second.back());
        it->second.pop_back();
    } else {
        surface = std::make_unique<renderer2d::SurfaceRGBA>(width, height);
    }
    
    return std::shared_ptr<renderer2d::SurfaceRGBA>(surface.release(), SurfaceDeleter{shared_from_this()});
}

void SurfacePool::prepare(std::uint32_t width, std::uint32_t height, std::size_t worker_count) {
    std::size_t count = m_policy.resolve(width, height, worker_count);
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_prepared_width = width;
    m_prepared_height = height;
    
    PoolKey key{width, height};
    auto& bucket = m_pool[key];
    
    while (bucket.size() < count) {
        bucket.push_back(std::make_unique<renderer2d::SurfaceRGBA>(width, height));
    }
}

std::shared_ptr<renderer2d::SurfaceRGBA> SurfacePool::acquire_prepared() {
    return acquire(m_prepared_width, m_prepared_height);
}

void SurfacePool::release(renderer2d::SurfaceRGBA* surface) {
    if (!surface) return;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    PoolKey key{surface->width(), surface->height()};
    auto& bucket = m_pool[key];
    
    std::unique_ptr<renderer2d::SurfaceRGBA> surface_ptr(surface);
    if (bucket.size() < m_policy.max_surfaces) {
        bucket.push_back(std::move(surface_ptr));
    }
}

void SurfacePool::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pool.clear();
}

void SurfacePool::prewarm(std::uint32_t width, std::uint32_t height, std::size_t count) {
    if (width == 0 || height == 0 || count == 0) {
        return;
    }

    std::vector<std::shared_ptr<renderer2d::SurfaceRGBA>> temp;
    temp.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        auto fb = acquire(width, height);
        if (fb) {
            fb->clear(renderer2d::Color::transparent());
            temp.push_back(std::move(fb));
        }
    }
    // Destructing temp automatically returns the surfaces to the pool
}

std::size_t SurfacePool::available_count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::size_t count = 0;
    for (const auto& [key, bucket] : m_pool) {
        count += bucket.size();
    }
    return count;
}

std::size_t SurfacePool::current_bytes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::size_t bytes = 0;
    for (const auto& [key, bucket] : m_pool) {
        for (const auto& surface : bucket) {
            if (surface) {
                bytes += surface->pixels().size() * sizeof(float) + surface->depth_buffer().size() * sizeof(float);
            }
        }
    }
    return bytes;
}

} // namespace tachyon
