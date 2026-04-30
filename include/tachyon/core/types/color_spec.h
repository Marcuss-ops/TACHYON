#pragma once

#include "tachyon/core/math/vector3.h"
#include <cstdint>

namespace tachyon {

/**
 * @brief Simple 8-bit RGBA color specification used in project schemas.
 */
struct ColorSpec {
    std::uint8_t r{255}, g{255}, b{255}, a{255};

    ColorSpec() = default;
    ColorSpec(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}

    [[nodiscard]] bool operator==(const ColorSpec& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    [[nodiscard]] bool operator!=(const ColorSpec& other) const {
        return !(*this == other);
    }

    [[nodiscard]] math::Vector3 to_vector3() const {
        return {r / 255.0f, g / 255.0f, b / 255.0f};
    }
};

} // namespace tachyon
