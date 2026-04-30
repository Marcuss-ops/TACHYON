#include "tachyon/core/scene/state/evaluated_camera2d_state.h"
#include "tachyon/core/math/matrix3x3.h"

namespace tachyon {

void EvaluatedCamera2D::recalculate_matrices() {
    using namespace math;
    
    Matrix3x3 translate = Matrix3x3::make_translation(position);
    Matrix3x3 rotate = Matrix3x3::make_rotation(rotation);
    Matrix3x3 scale_mat = Matrix3x3::make_scale(scale.x * zoom, scale.y * zoom);
    Matrix3x3 anchor = Matrix3x3::make_translation(anchor_point);
    Matrix3x3 anchor_inv = Matrix3x3::make_translation(-anchor_point);
    
    view_matrix = anchor_inv * scale_mat * rotate * translate * anchor;
    inverse_view_matrix = view_matrix.inverse();
}

} // namespace tachyon
