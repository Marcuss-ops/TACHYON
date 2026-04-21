#pragma once

#include <memory_resource>
#include <cstddef>
#include <vector>

namespace tachyon {

/**
 * @brief High-performance arena for per-frame transient allocations.
 * 
 * Uses a monotonic buffer resource to provide extremely fast allocations 
 * without individual deallocations. The entire arena is cleared at the 
 * end of each frame.
 */
class FrameArena {
public:
    explicit FrameArena(std::size_t initial_size = 1024 * 1024) // 1MB default
        : m_buffer(initial_size),
          m_resource(m_buffer.data(), m_buffer.size(), std::pmr::get_default_resource()) {}

    /**
     * @brief Allocates an object of type T in the arena.
     */
    template <typename T, typename... Args>
    T* allocate(Args&&... args) {
        void* ptr = m_resource.allocate(sizeof(T), alignof(T));
        return new (ptr) T(std::forward<Args>(args)...);
    }

    /**
     * @brief Allocates an array of type T in the arena.
     */
    template <typename T>
    T* allocate_array(std::size_t count) {
        return static_cast<T*>(m_resource.allocate(sizeof(T) * count, alignof(T)));
    }

    /**
     * @brief Returns a polymorphic allocator linked to this arena.
     */
    [[nodiscard]] std::pmr::polymorphic_allocator<std::byte> allocator() noexcept {
        return &m_resource;
    }

    /**
     * @brief Clears the arena for reuse. 
     * Note: This does NOT call destructors for objects allocated in the arena.
     */
    void reset() {
        m_resource.release();
    }

private:
    std::vector<std::byte> m_buffer;
    std::pmr::monotonic_buffer_resource m_resource;
};

} // namespace tachyon
