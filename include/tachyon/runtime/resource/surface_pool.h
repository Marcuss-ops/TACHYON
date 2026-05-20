#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/runtime/policy/surface_pool_policy.h"

#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace tachyon {

/**
 * @brief Unified resource manager for SurfaceRGBA objects.
 * 
 * Consolidates memory reuse logic for both fixed-resolution render passes 
 * and ad-hoc surface requirements. Uses a policy-based approach to balance 
 * performance and memory pressure.
 */
class SurfacePool : public std::enable_shared_from_this<SurfacePool> {
public:
    explicit SurfacePool(const runtime::SurfacePoolPolicy& policy = {});
    ~SurfacePool() = default;

    /**
     * @brief Acquires a surface of the given dimensions.
     * The surface is automatically returned to the pool when the shared_ptr goes out of scope.
     */
    std::shared_ptr<renderer2d::SurfaceRGBA> acquire(std::uint32_t width, std::uint32_t height);

    /**
     * @brief Pre-allocates surfaces for a specific resolution and concurrency level.
     */
    void prepare(std::uint32_t width, std::uint32_t height, std::size_t worker_count);

    /**
     * @brief Acquires a surface from the "prepared" resolution.
     */
    std::shared_ptr<renderer2d::SurfaceRGBA> acquire_prepared();

    /**
     * @brief Clears all pooled surfaces.
     */
    void clear();

    /**
     * @brief Prewarms the pool by allocating and clearing surfaces to touch memory.
     */
    void prewarm(std::uint32_t width, std::uint32_t height, std::size_t count);

    /**
     * @brief Returns the number of pooled/available surfaces.
     */
    [[nodiscard]] std::size_t available_count() const;

    /**
     * @brief Returns the total bytes allocated by the pooled surfaces.
     */
    [[nodiscard]] std::size_t current_bytes() const;

    /**
     * @brief Updates the pool policy.
     */
    void set_policy(const runtime::SurfacePoolPolicy& policy) { m_policy = policy; }

private:
    friend struct SurfaceDeleter;
    void release(renderer2d::SurfaceRGBA* surface);

    struct PoolKey {
        std::uint32_t width;
        std::uint32_t height;
        
        bool operator==(const PoolKey& other) const {
            return width == other.width && height == other.height;
        }
    };

    struct PoolKeyHash {
        std::size_t operator()(const PoolKey& k) const {
            return (static_cast<std::size_t>(k.width) << 32) | static_cast<std::size_t>(k.height);
        }
    };

    mutable std::mutex m_mutex;
    runtime::SurfacePoolPolicy m_policy;
    
    std::unordered_map<PoolKey, std::vector<std::unique_ptr<renderer2d::SurfaceRGBA>>, PoolKeyHash> m_pool;
    
    std::uint32_t m_prepared_width{0};
    std::uint32_t m_prepared_height{0};
};

struct SurfaceDeleter {
    std::shared_ptr<SurfacePool> pool;
    void operator()(renderer2d::SurfaceRGBA* s) const {
        if (pool) {
            pool->release(s);
        } else {
            delete s;
        }
    }
};

} // namespace tachyon
