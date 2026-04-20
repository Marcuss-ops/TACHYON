#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include <cstdint>
#include <vector>

namespace tachyon {

struct ColorSpec {
    std::uint8_t r{255}, g{255}, b{255}, a{255};

    [[nodiscard]] math::Vector3 to_vector3() const {
        return {r / 255.0f, g / 255.0f, b / 255.0f};
    }
};

struct GradientStop {
    float offset{0.0f};
    ColorSpec color{255, 255, 255, 255};
};

enum class GradientType {
    Linear,
    Radial
};

struct GradientSpec {
    GradientType type{GradientType::Linear};
    math::Vector2 start{0.0f, 0.0f};
    math::Vector2 end{100.0f, 0.0f};
    float radial_radius{100.0f};
    std::vector<GradientStop> stops;
};

} // namespace tachyon
