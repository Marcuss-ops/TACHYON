#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace tachyon {

class CacheKeyBuilder {
public:
    void add_u64(std::uint64_t value) noexcept {
        mix(value);
    }

    void add_u32(std::uint32_t value) noexcept {
        mix(static_cast<std::uint64_t>(value));
    }

    void add_bool(bool value) noexcept {
        mix(value ? 1ULL : 0ULL);
    }

    void add_string(std::string_view value) noexcept {
        add_u64(static_cast<std::uint64_t>(value.size()));
        for (unsigned char ch : value) {
            mix(static_cast<std::uint64_t>(ch));
        }
    }

    void add_bytes(std::span<const std::byte> bytes) noexcept {
        add_u64(static_cast<std::uint64_t>(bytes.size()));
        for (std::byte b : bytes) {
            mix(static_cast<std::uint64_t>(std::to_integer<unsigned char>(b)));
        }
    }

    [[nodiscard]] std::uint64_t finish() const noexcept {
        return avalanche(m_state ^ 0x9e3779b97f4a7c15ULL);
    }

private:
    std::uint64_t m_state{0xcbf29ce484222325ULL};

    static constexpr std::uint64_t avalanche(std::uint64_t value) noexcept {
        value ^= value >> 33U;
        value *= 0xff51afd7ed558ccdULL;
        value ^= value >> 33U;
        value *= 0xc4ceb9fe1a85ec53ULL;
        value ^= value >> 33U;
        return value;
    }

    void mix(std::uint64_t value) noexcept {
        m_state ^= avalanche(value + 0x9e3779b97f4a7c15ULL);
        m_state *= 0x100000001b3ULL;
    }
};

} // namespace tachyon

