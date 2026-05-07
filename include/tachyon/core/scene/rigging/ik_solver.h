#pragma once
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/math/quaternion.h"

namespace tachyon::scene {

struct IKResult {
    math::Quaternion root_rotation;
    math::Quaternion joint_rotation;
    bool reached{true};
};

struct IKChain {
    math::Vector3 root_pos;
    math::Vector3 joint_pos;
    math::Vector3 effector_pos;
    
    math::Quaternion root_rot_fk;
    math::Quaternion joint_rot_fk;
    
    // Config
    float bone_a_length{1.0f};
    float bone_b_length{1.0f};
    float ik_blend{1.0f}; // 0 = FK, 1 = IK
    
    // Result
    math::Quaternion root_rot_final;
    math::Quaternion joint_rot_final;
};

class IKSolver {
public:
    /**
     * Solves a 2-bone IK chain.
     * @param root_pos World position of the first joint.
     * @param target_pos World position of the target.
     * @param a Length of the first bone (root to joint).
     * @param b Length of the second bone (joint to effector).
     * @param pole_pos World position for the bending direction (pole vector).
     */
    static IKResult solve_2_bone(
        const math::Vector3& root_pos,
        const math::Vector3& target_pos,
        float a,
        float b,
        const math::Vector3& pole_pos);
        
    /**
     * Solves and blends an IK chain with its FK targets.
     */
    static void solve_and_blend_chain(
        IKChain& chain,
        const math::Vector3& target_pos,
        const math::Vector3& pole_pos);
};

} // namespace tachyon::scene
