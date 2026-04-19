#include "tachyon/core/math/quaternion.h"
#include "tachyon/core/math/matrix4x4.h"

namespace tachyon {
namespace math {

Matrix4x4 Quaternion::to_matrix() const {
    return Matrix4x4::from_quaternion(*this);
}

} // namespace math
} // namespace tachyon
