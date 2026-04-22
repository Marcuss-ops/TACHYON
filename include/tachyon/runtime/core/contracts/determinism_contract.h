/**
 * @file determinism_contract.h
 * @brief Formal execution contract for bit-identical rendering.
 */

#pragma once

#include <cstdint>

namespace tachyon {

/**
 * @brief Defines the exact mathematical and procedural constraints for a render.
 */
struct DeterminismContract {
    enum class FloatingPointMode : std::uint16_t {
        Strict = 0,    ///< Enforces strict IEEE 754 compliance.
        Relaxed = 1    ///< Allows minor platform optimizations (e.g. fast-math).
    };

    enum class FmaPolicy : std::uint16_t {
        Disable = 0,   ///< Explicitly disable Fused Multiply-Add to prevent drift.
        Enable = 1     ///< Use FMA where available (risk of cross-build drift).
    };

    enum class DenormalPolicy : std::uint16_t {
        FlushToZero = 0, ///< Treat denormals as zero for performance.
        Preserve = 1    ///< Maintain full denormal precision (slower).
    };

    enum class RoundingMode : std::uint16_t {
        Nearest = 0,
        TowardZero = 1,
        Upward = 2,
        Downward = 3
    };

    enum class PrngKind : std::uint16_t {
        SplitMix64 = 0,
        PCG32 = 1
    };

    FloatingPointMode fp_mode{FloatingPointMode::Strict};
    FmaPolicy fma_policy{FmaPolicy::Disable};
    DenormalPolicy denormal_policy{DenormalPolicy::FlushToZero};
    RoundingMode rounding_mode{RoundingMode::Nearest};
    PrngKind prng{PrngKind::SplitMix64};

    std::uint64_t seed{0};
    std::uint32_t layout_version{1};
    std::uint32_t compiler_version{1};

    /**
     * @brief Produces a cryptographic-style fingerprint of the contract.
     * Any change in these parameters must result in a different fingerprint.
     */
    [[nodiscard]] std::uint64_t fingerprint() const noexcept {
        std::uint64_t h = 0x9e3779b97f4a7c15ULL;
        const auto mix = [&h](std::uint64_t v) noexcept {
            // XXHash-style mix
            h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6U) + (h >> 2U);
        };

        mix(static_cast<std::uint64_t>(fp_mode));
        mix(static_cast<std::uint64_t>(fma_policy));
        mix(static_cast<std::uint64_t>(denormal_policy));
        mix(static_cast<std::uint64_t>(rounding_mode));
        mix(static_cast<std::uint64_t>(prng));
        mix(seed);
        mix(static_cast<std::uint64_t>(layout_version));
        mix(static_cast<std::uint64_t>(compiler_version));
        return h;
    }
};

} // namespace tachyon
