#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"

#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace tachyon::renderer2d {

/**
 * @brief Pool for reusing SurfaceRGBA memory buffers.
 * 
 * Minimizes allocations in the render loop by keeping a set of 
 * pre-allocated surfaces grouped by their dimensions.
 */
class SurfacePool : public std::enable_shared_from_this<SurfacePool> {
public:
    SurfacePool() = default;
    ~SurfacePool() = default;

    /**
     * Acquire a surface of the given dimensions.
     * The returned shared_ptr has a custom deleter that returns the surface 
     * to this pool instead of deleting it.
     */
    std::shared_ptr<SurfaceRGBA> acquire(std::uint32_t width, std::uint32_t height);

    /**
     * Clear all pooled memory.
     */
    void clear();

    void set_capacity(std::size_t count) { m_capacity = count; }

private:
    friend struct SurfaceDeleter;

    void release(SurfaceRGBA* surface);

    struct PoolEntry {
        std::uint32_t width;
        std::uint32_t height;
        
        bool operator==(const PoolEntry& other) const {
            return width == other.width && height == other.height;
        }
    };

    struct PoolEntryHash {
        std::size_t operator()(const PoolEntry& e) const {
            return (static_cast<std::size_t>(e.width) << 32) | static_cast<std::size_t>(e.height);
        }
    };

    std::mutex m_mutex;
    std::size_t m_capacity{64};
    std::unordered_map<PoolEntry, std::vector<std::unique_ptr<SurfaceRGBA>>, PoolEntryHash> m_pool;
};

struct SurfaceDeleter {
    std::shared_ptr<SurfacePool> pool;
    void operator()(SurfaceRGBA* s) const {
        if (pool) {
            pool->release(s);
        } else {
            delete s;
        }
    }
};

} // namespace tachyon::renderer2d