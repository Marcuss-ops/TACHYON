#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include <cstdint>
#include <vector>

namespace tachyon {

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
