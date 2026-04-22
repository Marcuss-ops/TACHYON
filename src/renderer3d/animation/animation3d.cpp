#include "tachyon/renderer3d/animation/animation3d.h"
#include <algorithm>
#include <cmath>

namespace tachyon::renderer3d {

const AnimationInterpolator::Keyframe* AnimationInterpolator::TransformKeyframes::find_position_key(double time) const {
    if (position_keys.empty()) return nullptr;
    for (std::size_t i = 0; i < position_keys.size() - 1; ++i) {
        if (time >= position_keys[i].time_seconds && time < position_keys[i + 1].time_seconds) {
            return &position_keys[i];
        }
    }
    return &position_keys.back();
}

const AnimationInterpolator::Keyframe* AnimationInterpolator::TransformKeyframes::find_rotation_key(double time) const {
    if (rotation_keys.empty()) return nullptr;
    for (std::size_t i = 0; i < rotation_keys.size() - 1; ++i) {
        if (time >= rotation_keys[i].time_seconds && time < rotation_keys[i + 1].time_seconds) {
            return &rotation_keys[i];
        }
    }
    return &rotation_keys.back();
}

const AnimationInterpolator::Keyframe* AnimationInterpolator::TransformKeyframes::find_scale_key(double time) const {
    if (scale_keys.empty()) return nullptr;
    for (std::size_t i = 0; i < scale_keys.size() - 1; ++i) {
        if (time >= scale_keys[i].time_seconds && time < scale_keys[i + 1].time_seconds) {
            return &scale_keys[i];
        }
    }
    return &scale_keys.back();
}

math::Transform3 AnimationInterpolator::interpolate(
    const std::vector<Keyframe>& keys,
    double time,
    InterpolationType default_type) 
{
    if (keys.empty()) return math::Transform3::identity();
    if (keys.size() == 1) {
        return math::Transform3{
            keys[0].position,
            keys[0].rotation,
            keys[0].scale
        };
    }

    const Keyframe* prev = nullptr;
    const Keyframe* next = nullptr;

    for (std::size_t i = 0; i < keys.size() - 1; ++i) {
        if (time >= keys[i].time_seconds && time < keys[i + 1].time_seconds) {
            prev = &keys[i];
            next = &keys[i + 1];
            break;
        }
    }

    if (!prev) {
        prev = &keys[keys.size() - 2];
        next = &keys[keys.size() - 1];
    }

    float range = static_cast<float>(next->time_seconds - prev->time_seconds);
    float t = range > 0.0f ? static_cast<float>((time - prev->time_seconds) / static_cast<double>(range)) : 0.0f;
    t = std::clamp(t, 0.0f, 1.0f);

    InterpolationType interp = prev->interpolation != InterpolationType::Linear ? prev->interpolation : default_type;
    if (interp == InterpolationType::Linear) {
        t = ease(t, prev->ease_in, prev->ease_out);
    }

    math::Vector3 pos = prev->position + (next->position - prev->position) * static_cast<float>(t);
    math::Quaternion rot = (prev->rotation * static_cast<float>(1.0 - t) + next->rotation * static_cast<float>(t)).normalized();
    math::Vector3 scale = prev->scale + (next->scale - prev->scale) * static_cast<float>(t);

    return math::Transform3{pos, rot, scale};
}

math::Transform3 AnimationInterpolator::interpolate_with_easing(
    const std::vector<Keyframe>& keys,
    double time) 
{
    return interpolate(keys, time, InterpolationType::Bezier);
}

math::Matrix4x4 AnimationInterpolator::interpolate_matrix(
    const std::vector<Keyframe>& pos_keys,
    const std::vector<Keyframe>& rot_keys,
    const std::vector<Keyframe>& scale_keys,
    double time) 
{
    math::Transform3 t;
    t = interpolate(pos_keys, time, InterpolationType::Linear);
    if (!rot_keys.empty()) {
        math::Quaternion r = interpolate(rot_keys, time, InterpolationType::Linear).rotation;
        t.rotation = r;
    }
    if (!scale_keys.empty()) {
        math::Vector3 s = interpolate(scale_keys, time, InterpolationType::Linear).scale;
        t.scale = s;
    }
    return t.to_matrix();
}

float AnimationInterpolator::ease(float t, float ease_in, float ease_out) {
    if (ease_in <= 0.0f && ease_out <= 0.0f) return t;
    if (ease_in > 0.0f) {
        float c = 1.0f - ease_in;
        t = (c * t * t * t + ease_in * t);
    }
    if (ease_out > 0.0f) {
        float c = 1.0f - ease_out;
        float r = 1.0f - t;
        t = 1.0f - (c * r * r * r + ease_out * r);
    }
    return t;
}

float AnimationInterpolator::bezier(float t, const math::Vector2& p0, const math::Vector2& p1) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    return uu * u * p0.x + 3.0f * uu * t * p0.y + 3.0f * u * tt * p1.y + tt * t * p1.x;
}

std::size_t HierarchySystem::Skeleton::find_bone(const std::string& id) const {
    auto it = bone_indices.find(id);
    if (it != bone_indices.end()) return it->second;
    return static_cast<std::size_t>(-1);
}

const HierarchySystem::Bone* HierarchySystem::Skeleton::find_bone_ptr(const std::string& id) const {
    std::size_t idx = find_bone(id);
    if (idx >= bones.size()) return nullptr;
    return &bones[idx];
}

void HierarchySystem::SkeletonPose::resize(std::size_t size) {
    joint_matrices.resize(size);
    inverse_joint_matrices.resize(size);
    dirty_flags.resize(size, true);
}

void HierarchySystem::SkeletonPose::mark_dirty(std::size_t index) {
    if (index < dirty_flags.size()) dirty_flags[index] = true;
}

void HierarchySystem::SkeletonPose::mark_all_dirty() {
    for (std::size_t i = 0; i < dirty_flags.size(); ++i) {
        dirty_flags[i] = true;
    }
}

bool HierarchySystem::SkeletonPose::is_dirty(std::size_t index) const {
    if (index >= dirty_flags.size()) return true;
    return dirty_flags[index];
}

HierarchySystem::SkeletonPose HierarchySystem::evaluate_pose(
    const Skeleton& skeleton,
    const std::vector<math::Transform3>& local_poses) 
{
    SkeletonPose pose;
    pose.resize(skeleton.bones.size());

    for (std::size_t i = 0; i < skeleton.bones.size(); ++i) {
        const auto& bone = skeleton.bones[i];
        if (i >= local_poses.size()) {
            continue;
        }
        math::Matrix4x4 local = local_poses[i].to_matrix();
        pose.joint_matrices[i] = local * bone.inverse_bind_pose;
    }

    return pose;
}

math::Matrix4x4 HierarchySystem::compute_world_matrix(
    const Bone& bone,
    const SkeletonPose& pose) 
{
    (void)bone;
    if (pose.joint_matrices.empty()) {
        return math::Matrix4x4::identity();
    }
    return pose.joint_matrices.front();
}

void HierarchySystem::update_hierarchy(
    Skeleton& skeleton,
    SkeletonPose& pose) 
{
    for (std::size_t i = 0; i < skeleton.bones.size(); ++i) {
        auto& bone = skeleton.bones[i];
        if (bone.is_root) {
            pose.joint_matrices[i] = bone.world_transform.to_matrix();
        } else {
            const Bone* parent = skeleton.find_bone_ptr(bone.parent_id);
            if (parent) {
                std::size_t pi = skeleton.find_bone(bone.parent_id);
                pose.joint_matrices[i] = bone.local_transform.to_matrix() * pose.joint_matrices[pi];
            }
        }
        pose.joint_matrices[i] = pose.joint_matrices[i] * bone.inverse_bind_pose;
        pose.dirty_flags[i] = false;
    }
}

math::Transform3 CameraRig::evaluate_rig(
    const RigConfig& rig,
    double time) 
{
    (void)time;
    math::Transform3 result{math::Transform3::identity()};

    // If we have nodes in a hierarchy, the world transform of the camera node is the truth
    if (!rig.nodes.empty()) {
        const auto& camera_node = rig.nodes.back();
        result = camera_node.local_transform;
        // In a real rig, we'd multiply by parent transforms here or use the Constraint solver result
    }

    return result;
}

void CameraRig::update_target(
    RigConfig& rig,
    const math::Vector3& target_position) 
{
    rig.target = target_position;
}

math::Vector3 CameraRig::compute_camera_position(
    const RigConfig& rig) 
{
    math::Vector3 pos = math::Vector3::zero();
    float total_weight = 0.0f;

    for (const auto& node : rig.nodes) {
        math::Vector3 node_pos = node.local_transform.position;
        float weight = 1.0f;
        if (node.type == CameraNode::NodeType::Crane) weight = 0.5f;
        pos = pos + node_pos * weight;
        total_weight += weight;
    }

    if (total_weight > 0.0f) pos = pos / total_weight;
    return pos;
}

math::Matrix4x4 CameraRig::compute_view_matrix(
    const math::Vector3& camera_position,
    const math::Vector3& target,
    const math::Vector3& up) 
{
    math::Vector3 forward = (target - camera_position).normalized();
    math::Vector3 right = math::Vector3::cross(forward, up).normalized();
    math::Vector3 cam_up = math::Vector3::cross(right, forward);

    math::Matrix4x4 view;
    view[0] = right.x; view[1] = right.y; view[2] = right.z; view[3] = -math::Vector3::dot(right, camera_position);
    view[4] = cam_up.x; view[5] = cam_up.y; view[6] = cam_up.z; view[7] = -math::Vector3::dot(cam_up, camera_position);
    view[8] = forward.x; view[9] = forward.y; view[10] = forward.z; view[11] = -math::Vector3::dot(forward, camera_position);
    view[12] = 0.0f; view[13] = 0.0f; view[14] = 0.0f; view[15] = 1.0f;

    return view;
}

math::Matrix4x4 CameraRig::compute_projection_matrix(
    float fov,
    float aspect,
    float near_plane,
    float far_plane) 
{
    float f = 1.0f / std::tan(fov * 0.5f);
    float range_inv = 1.0f / (near_plane - far_plane);

    math::Matrix4x4 proj;
    proj[0] = f / aspect; proj[1] = 0.0f; proj[2] = 0.0f; proj[3] = 0.0f;
    proj[4] = 0.0f; proj[5] = f; proj[6] = 0.0f; proj[7] = 0.0f;
    proj[8] = 0.0f; proj[9] = 0.0f; proj[10] = (far_plane + near_plane) * range_inv; proj[11] = 2.0f * far_plane * near_plane * range_inv;
    proj[12] = 0.0f; proj[13] = 0.0f; proj[14] = -1.0f; proj[15] = 0.0f;

    return proj;
}

} // namespace tachyon::renderer3d
