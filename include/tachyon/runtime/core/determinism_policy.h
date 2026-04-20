#pragma once

#include <cstdint>

namespace tachyon {

struct DeterminismPolicy {
    enum class FloatingPointMode : std::uint16_t {
        Strict = 0,
        Relaxed = 1
    };

    enum class TraversalOrder : std::uint16_t {
        StableTopological = 0
    };

    enum class PrngKind : std::uint16_t {
        PCG32 = 0
    };

    FloatingPointMode fp_mode{FloatingPointMode::Strict};
    TraversalOrder traversal_order{TraversalOrder::StableTopological};
    PrngKind prng{PrngKind::PCG32};
    std::uint64_t seed{0};
    std::uint32_t fps{60};
    std::uint32_t layout_version{1};
    std::uint32_t compiler_version{1};

    [[nodiscard]] std::uint64_t fingerprint() const noexcept {
        std::uint64_t h = 0x9e3779b97f4a7c15ULL;
        const auto mix = [&h](std::uint64_t v) noexcept {
            h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6U) + (h >> 2U);
        };

        mix(static_cast<std::uint64_t>(fp_mode));
        mix(static_cast<std::uint64_t>(traversal_order));
        mix(static_cast<std::uint64_t>(prng));
        mix(seed);
        mix(static_cast<std::uint64_t>(fps));
        mix(static_cast<std::uint64_t>(layout_version));
        mix(static_cast<std::uint64_t>(compiler_version));
        return h;
    }
};

} // namespace tachyon

