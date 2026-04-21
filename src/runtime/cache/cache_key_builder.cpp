#include "tachyon/runtime/cache/cache_key_builder.h"

namespace tachyon {

void CacheKeyBuilder::add_u64(std::uint64_t value) noexcept {
    mix(value);
}

void CacheKeyBuilder::add_u32(std::uint32_t value) noexcept {
    mix(static_cast<std::uint64_t>(value));
}

void CacheKeyBuilder::add_bool(bool value) noexcept {
    mix(value ? 1ULL : 0ULL);
}

void CacheKeyBuilder::add_string(std::string_view value) noexcept {
    add_u64(static_cast<std::uint64_t>(value.size()));
    for (unsigned char ch : value) {
        mix(static_cast<std::uint64_t>(ch));
    }
}

void CacheKeyBuilder::add_bytes(std::span<const std::byte> bytes) noexcept {
    add_u64(static_cast<std::uint64_t>(bytes.size()));
    for (std::byte b : bytes) {
        mix(static_cast<std::uint64_t>(std::to_integer<unsigned char>(b)));
    }
}

std::uint64_t CacheKeyBuilder::finish() const noexcept {
    return avalanche(m_state ^ 0x9e3779b97f4a7c15ULL);
}

void CacheKeyBuilder::mix(std::uint64_t value) noexcept {
    m_state ^= avalanche(value + 0x9e3779b97f4a7c15ULL);
    m_state *= 0x100000001b3ULL;
}

} // namespace tachyon
