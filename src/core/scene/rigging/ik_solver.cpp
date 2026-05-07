#include "tachyon/core/scene/rigging/ik_solver.h"
#include "tachyon/core/math/math_utils.h"
#include <algorithm>
#include <cmath>

namespace tachyon::scene {

namespace {

math::Quaternion slerp_quaternion(const math::Quaternion& a, const math::Quaternion& b, float t) {
    float dot = math::Quaternion::dot(a, b);
    math::Quaternion end = b;
    if (dot < 0.0f) {
        dot = -dot;
        end = {-b.x, -b.y, -b.z, -b.w};
    }
    if (dot > 0.9995f) {
        return math::Quaternion{
            a.x + (end.x - a.x) * t,
            a.y + (end.y - a.y) * t,
            a.z + (end.z - a.z) * t,
            a.w + (end.w - a.w) * t
        }.normalized();
    }
    const float theta_0 = std::acos(std::clamp(dot, -1.0f, 1.0f));
    const float theta = theta_0 * t;
    const float sin_theta = std::sin(theta);
    const float sin_theta_0 = std::sin(theta_0);
    const float s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
    const float s1 = sin_theta / sin_theta_0;
    return {
        a.x * s0 + end.x * s1,
        a.y * s0 + end.y * s1,
        a.z * s0 + end.z * s1,
        a.w * s0 + end.w * s1
    };
}

} // namespace

IKResult IKSolver::solve_2_bone(
    const math::Vector3& root_pos,
    const math::Vector3& target_pos,
    float a,
    float b,
    const math::Vector3& pole_pos) {
    
    IKResult result;
    const math::Vector3 to_target = target_pos - root_pos;
    float dist = to_target.length();
    
    // Clamp distance to reach
    if (dist > (a + b)) {
        dist = a + b;
        result.reached = false;
    }
    if (dist < std::abs(a - b)) {
        dist = std::abs(a - b);
        result.reached = false;
    }

    // Law of Cosines
    float cos_c = (a * a + b * b - dist * dist) / (2.0f * a * b);
    float angle_c = std::acos(std::clamp(cos_c, -1.0f, 1.0f));

    float cos_a = (a * a + dist * dist - b * b) / (2.0f * a * dist);
    float angle_a = std::acos(std::clamp(cos_a, -1.0f, 1.0f));

    // Plane orientation
    math::Vector3 forward = to_target.normalized();
    math::Vector3 pole_dir = (pole_pos - root_pos).normalized();
    math::Vector3 side = math::Vector3::cross(forward, pole_dir);
    if (side.length_squared() < 1e-6f) {
        // Fallback if pole is collinear
        side = math::Vector3::cross(forward, {0, 1, 0});
        if (side.length_squared() < 1e-6f) side = math::Vector3::cross(forward, {1, 0, 0});
    }
    side = side.normalized();
    math::Vector3 up = math::Vector3::cross(side, forward).normalized();

    // Rotations
    result.root_rotation = math::Quaternion::from_axis_angle(side, angle_a) * math::Quaternion::look_at({0,0,0}, forward, up);
    result.joint_rotation = math::Quaternion::from_axis_angle(side, -(static_cast<float>(math::kPi) - angle_c));

    return result;
}

void IKSolver::solve_and_blend_chain(
    IKChain& chain,
    const math::Vector3& target_pos,
    const math::Vector3& pole_pos) 
{
    if (chain.ik_blend <= 0.0f) {
        chain.root_rot_final = chain.root_rot_fk;
        chain.joint_rot_final = chain.joint_rot_fk;
        return;
    }
    
    IKResult ik = solve_2_bone(
        chain.root_pos, target_pos,
        chain.bone_a_length, chain.bone_b_length,
        pole_pos);
        
    if (chain.ik_blend >= 1.0f) {
        chain.root_rot_final = ik.root_rotation;
        chain.joint_rot_final = ik.joint_rotation;
    } else {
        chain.root_rot_final = slerp_quaternion(chain.root_rot_fk, ik.root_rotation, chain.ik_blend);
        chain.joint_rot_final = slerp_quaternion(chain.joint_rot_fk, ik.joint_rotation, chain.ik_blend);
    }
}

} // namespace tachyon::scene
