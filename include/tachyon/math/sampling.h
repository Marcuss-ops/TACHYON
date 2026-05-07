#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace tachyon::math {

/**
 * @brief Performs bilinear sampling on a 2D grid.
 * @tparam T Data type of the grid elements.
 * @tparam Sampler A callable with signature T(int x, int y) providing boundary-aware access.
 */
template <typename Sampler>
inline auto sample_bilinear(float x, float y, Sampler&& sampler) -> decltype(sampler(0, 0)) {
    using T = decltype(sampler(0, 0));
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const float tx = x - static_cast<float>(x0);
    const float ty = y - static_cast<float>(y0);

    const T v00 = sampler(x0, y0);
    const T v10 = sampler(x0 + 1, y0);
    const T v01 = sampler(x0, y0 + 1);
    const T v11 = sampler(x0 + 1, y0 + 1);

    // Using a generic lerp-style approach that works for float, Color, etc.
    // Assumes T supports T operator*(float) and T operator+(const T&).
    const T top = v00 * (1.0f - tx) + v10 * tx;
    const T bottom = v01 * (1.0f - tx) + v11 * tx;
    return top * (1.0f - ty) + bottom * ty;
}

/**
 * @brief Helper for clamping coordinates to a range [0, limit-1].
 */
inline int clamp_coord(int v, int limit) {
    return std::clamp(v, 0, limit - 1);
}

} // namespace tachyon::math
