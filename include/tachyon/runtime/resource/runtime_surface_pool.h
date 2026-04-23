#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include <vector>
#include <memory>
#include <mutex>

namespace tachyon::runtime {

/**
 * @brief Thread-safe pool for reusing SurfaceRGBA (Framebuffer) objects.
 * Prevents frequent allocations and memory fragmentation.
 */
class RuntimeSurfacePool {
public:
    RuntimeSurfacePool(std::uint32_t width, std::uint32_t height, std::size_t initial_count = 10)
        : m_width(width), m_height(height) {
        for (std::size_t i = 0; i < initial_count; ++i) {
            m_available_surfaces.push_back(std::make_unique<renderer2d::SurfaceRGBA>(width, height));
        }
    }

    /**
     * @brief Acquires a surface from the pool.
     * If the pool is empty, returns nullptr or could optionally allocate more (up to a budget).
     */
    std::unique_ptr<renderer2d::SurfaceRGBA> acquire() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_available_surfaces.empty()) {
            return nullptr; 
        }
        auto surface = std::move(m_available_surfaces.back());
        m_available_surfaces.pop_back();
        return surface;
    }

    /**
     * @brief Releases a surface back into the pool for reuse.
     */
    void release(std::unique_ptr<renderer2d::SurfaceRGBA> surface) {
        if (!surface) return;
        
        // Ensure the surface is the right size before returning to pool
        if (surface->width() != m_width || surface->height() != m_height) {
            surface->reset(m_width, m_height);
        }
        
        std::lock_guard<std::mutex> lock(m_mutex);
        m_available_surfaces.push_back(std::move(surface));
    }

    [[nodiscard]] std::size_t available_count() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_available_surfaces.size();
    }

private:
    std::uint32_t m_width;
    std::uint32_t m_height;
    std::vector<std::unique_ptr<renderer2d::SurfaceRGBA>> m_available_surfaces;
    mutable std::mutex m_mutex;
};

} // namespace tachyon::runtime