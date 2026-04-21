#pragma once

#include <cstddef>
#include <memory_resource>
#include <type_traits>
#include <vector>

namespace tachyon {

class FrameArena {
public:
    static constexpr std::size_t kDefaultBytes = 4U * 1024U * 1024U;

    explicit FrameArena(std::size_t initial_size = kDefaultBytes)
        : m_buffer(initial_size),
          m_resource(m_buffer.data(), m_buffer.size(), std::pmr::get_default_resource()) {}

    void reset() noexcept {
        m_resource.release();
    }

    [[nodiscard]] std::pmr::memory_resource* resource() noexcept {
        return &m_resource;
    }

    template <typename T>
    [[nodiscard]] std::pmr::polymorphic_allocator<T> allocator() noexcept {
        return std::pmr::polymorphic_allocator<T>{resource()};
    }

private:
    std::vector<std::byte> m_buffer;
    std::pmr::monotonic_buffer_resource m_resource;
};

} // namespace tachyon
