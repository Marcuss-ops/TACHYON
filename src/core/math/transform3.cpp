#include "tachyon/core/math/transform3.h"

#include <cmath>

namespace tachyon {
namespace math {

Matrix4x4 Transform3::to_matrix() const {
    return math::compose_trs(position, rotation, scale);
}

Matrix4x4 Transform3::to_inverse_matrix() const {
    return to_matrix().inverse_affine();
}

} // namespace math
} // namespace tachyon
