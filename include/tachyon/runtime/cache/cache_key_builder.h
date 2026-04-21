#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace tachyon {

class CacheKeyBuilder {
public:
    void add_u64(std::uint64_t value) noexcept;
    void add_u32(std::uint32_t value) noexcept;
    void add_f64(double value) noexcept;
    void add_f32(float value) noexcept;
    void add_bool(bool value) noexcept;
    void add_string(std::string_view value) noexcept;
    void add_bytes(std::span<const std::byte> bytes) noexcept;

    void enable_manifest(bool enabled) noexcept { m_manifest_enabled = enabled; }
    [[nodiscard]] const std::string& manifest() const noexcept { return m_manifest; }

    [[nodiscard]] std::uint64_t finish() const noexcept;

private:
    std::uint64_t m_state{0xcbf29ce484222325ULL};
    std::string m_manifest;
    bool m_manifest_enabled{false};

    static constexpr std::uint64_t avalanche(std::uint64_t value) noexcept {
        value ^= value >> 33U;
        value *= 0xff51afd7ed558ccdULL;
        value ^= value >> 33U;
        value *= 0xc4ceb9fe1a85ec53ULL;
        value ^= value >> 33U;
        return value;
    }

    void mix(std::uint64_t value) noexcept;
};

} // namespace tachyon

