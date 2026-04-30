#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/quaternion.h"
#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/math/transform3.h"

#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>

namespace tachyon::renderer3d {

class AnimationInterpolator {
public:
    enum class InterpolationType {
        Linear,
        Bezier,
        Hold,
        Cubic
    };

    struct Keyframe {
        double time_seconds{0.0};
        math::Vector3 position{math::Vector3::zero()};
        math::Quaternion rotation{math::Quaternion::identity()};
        math::Vector3 scale{1.0f, 1.0f, 1.0f};
        InterpolationType interpolation{InterpolationType::Linear};
        
        float ease_in{0.0f};
        float ease_out{0.0f};
    };

    struct TransformKeyframes {
        std::string bone_id;
        std::vector<Keyframe> position_keys;
        std::vector<Keyframe> rotation_keys;
        std::vector<Keyframe> scale_keys;

        const Keyframe* find_position_key(double time) const;
        const Keyframe* find_rotation_key(double time) const;
        const Keyframe* find_scale_key(double time) const;
    };

    struct AnimationClip {
        std::string name;
        double duration_seconds{0.0};
        double ticks_per_second{24.0};
        std::vector<TransformKeyframes> tracks;
    };

    static math::Transform3 interpolate(
        const std::vector<Keyframe>& keys,
        double time,
        InterpolationType default_type);

    static math::Transform3 interpolate_with_easing(
        const std::vector<Keyframe>& keys,
        double time);

    static math::Matrix4x4 interpolate_matrix(
        const std::vector<Keyframe>& pos_keys,
        const std::vector<Keyframe>& rot_keys,
        const std::vector<Keyframe>& scale_keys,
        double time);

    static float ease(float t, float ease_in, float ease_out);
    static float bezier(float t, const math::Vector2& p0, const math::Vector2& p1);
};

class HierarchySystem {
public:
    struct Bone {
        std::string id;
        std::string name;
        std::string parent_id;
        math::Transform3 local_transform{math::Transform3::identity()};
        math::Transform3 world_transform{math::Transform3::identity()};
        math::Matrix4x4 inverse_bind_pose;
        bool is_root{false};
    };

    struct Skeleton {
        std::string name;
        std::vector<Bone> bones;
        std::unordered_map<std::string, std::size_t> bone_indices;

        std::size_t find_bone(const std::string& id) const;
        const Bone* find_bone_ptr(const std::string& id) const;
    };

    struct SkeletonPose {
        std::vector<math::Matrix4x4> joint_matrices;
        std::vector<math::Matrix4x4> inverse_joint_matrices;
        std::vector<bool> dirty_flags;

        void resize(std::size_t size);
        void mark_dirty(std::size_t index);
        void mark_all_dirty();
        bool is_dirty(std::size_t index) const;
    };

    static SkeletonPose evaluate_pose(
        const Skeleton& skeleton,
        const std::vector<math::Transform3>& local_poses);

    static math::Matrix4x4 compute_world_matrix(
        const Bone& bone,
        const SkeletonPose& pose);

    static void update_hierarchy(
        Skeleton& skeleton,
        SkeletonPose& pose);
};

class CameraRig {
public:
    struct CameraNode {
        std::string id;
        std::string name;
        math::Transform3 local_transform{math::Transform3::identity()};
        math::Vector3 local_anchor{math::Vector3::zero()};
        
        enum class NodeType {
            Root,
            Pan,
            Tilt,
            Track,
            Dolly,
            Crane
        };
        NodeType type{NodeType::Root};
        
        float min_limit{0.0f};
        float max_limit{0.0f};
        float damping{0.0f};
    };

    struct RigConfig {
        std::string name;
        std::vector<CameraNode> nodes;
        math::Vector3 target{math::Vector3::zero()};
        
        bool auto_focus{true};
        float field_of_view{60.0f};
        float focal_length{50.0f};
        
        std::string quality_tier{"high"};
    };

    static math::Transform3 evaluate_rig(
        const RigConfig& rig,
        double time);

    static void update_target(
        RigConfig& rig,
        const math::Vector3& target_position);

    static math::Vector3 compute_camera_position(
        const RigConfig& rig);

    static math::Matrix4x4 compute_view_matrix(
        const math::Vector3& camera_position,
        const math::Vector3& target,
        const math::Vector3& up);

    static math::Matrix4x4 compute_projection_matrix(
        float fov,
        float aspect,
        float near_plane,
        float far_plane);
};

} // namespace tachyon::renderer3d
