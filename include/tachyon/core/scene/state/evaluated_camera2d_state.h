#pragma once

#include "tachyon/core/math/matrix3x3.h"
#include "tachyon/core/math/vector2.h"

namespace tachyon {

struct EvaluatedCamera2D {
    math::Vector2 position;
    float rotation;
    math::Vector2 scale;
    math::Vector2 anchor_point;
    float zoom{1.0f};
    
    math::Matrix3x3 view_matrix;
    math::Matrix3x3 inverse_view_matrix;
    
    int viewport_width{1920};
    int viewport_height{1080};
    
    void recalculate_matrices();
};

} // namespace tachyon
