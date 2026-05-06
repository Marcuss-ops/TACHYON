#pragma once

#include <cstdint>
#include <string>
#include <utility>

namespace tachyon::runtime {

/**
 * @brief Typed wrapper for property-level cache keys.
 */
struct PropertyCacheKey {
    std::uint64_t value{0};

    explicit PropertyCacheKey(std::uint64_t v = 0) noexcept : value(v) {}
    bool operator==(const PropertyCacheKey&) const noexcept = default;
};

/**
 * @brief Typed wrapper for layer-level cache keys.
 */
struct LayerCacheKey {
    std::uint64_t value{0};

    explicit LayerCacheKey(std::uint64_t v = 0) noexcept : value(v) {}
    bool operator==(const LayerCacheKey&) const noexcept = default;
};

/**
 * @brief Typed wrapper for composition-level cache keys.
 */
struct CompositionCacheKey {
    std::uint64_t value{0};

    explicit CompositionCacheKey(std::uint64_t v = 0) noexcept : value(v) {}
    bool operator==(const CompositionCacheKey&) const noexcept = default;
};

} // namespace tachyon::runtime
