#include "tachyon/core/math/transform3.h"

namespace tachyon {
namespace math {

Matrix4x4 Transform3::to_matrix() const {
    Matrix4x4 T = Matrix4x4::translation(position);
    Matrix4x4 R = Matrix4x4::from_quaternion(rotation);
    Matrix4x4 S = Matrix4x4::scaling(scale);
    
    // T * R * S
    return T * (R * S);
}

Matrix4x4 Transform3::to_inverse_matrix() const {
    // (T * R * S)^-1 = S^-1 * R^-1 * T^-1
    
    Vector3 inv_scale = {
        (std::abs(scale.x) > 1e-6f) ? 1.0f / scale.x : 0.0f,
        (std::abs(scale.y) > 1e-6f) ? 1.0f / scale.y : 0.0f,
        (std::abs(scale.z) > 1e-6f) ? 1.0f / scale.z : 0.0f
    };
    
    Matrix4x4 invS = Matrix4x4::scaling(inv_scale);
    
    // Quaternion inverse (assuming normalized): q* = (-x,-y,-z, w)
    Quaternion invR_quat(-rotation.x, -rotation.y, -rotation.z, rotation.w);
    Matrix4x4 invR = Matrix4x4::from_quaternion(invR_quat);
    
    Matrix4x4 invT = Matrix4x4::translation(position * -1.0f);
    
    return invS * (invR * invT);
}

} // namespace math
} // namespace tachyon
