#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/math/algebra/vector3.h"
#include "tachyon/core/math/algebra/matrix4x4.h"
#include "tachyon/core/camera/camera_state.h"

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <unordered_map>

namespace tachyon::renderer3d {

class SceneCulling {
public:
    enum class CullTest {
        Inside,
        Outside,
        Intersecting
    };

    enum class LODLevel {
        Low,
        Medium,
        High,
        Ultra
    };

    struct BoundingBox {
        math::Vector3 min{math::Vector3::zero()};
        math::Vector3 max{math::Vector3::zero()};

        math::Vector3 center() const { return (min + max) * 0.5f; }
        math::Vector3 extents() const { return max - min; }
        float volume() const { auto e = extents(); return e.x * e.y * e.z; }
        bool is_empty() const { return min.x >= max.x || min.y >= max.y || min.z >= max.z; }

        static BoundingBox empty() { return {math::Vector3::zero(), math::Vector3::zero()}; }
        static BoundingBox from_center_extents(const math::Vector3& center, const math::Vector3& extents) {
            return {center - extents * 0.5f, center + extents * 0.5f};
        }
    };

    struct CullingResult {
        std::vector<std::size_t> visibleIndices;
        std::vector<std::size_t> culledIndices;
        std::size_t total_objects{0};
        std::size_t visible_count{0};
        std::size_t culled_count{0};
        double culling_time_ms{0.0};
    };

    struct LODResult {
        std::size_t layer_index{0};
        LODLevel level{LODLevel::High};
        float distance{0.0f};
        bool should_use_low_lod{false};
    };

    struct Frustum {
        math::Vector3 planes[6];
        math::Vector3 corners[8];

        static Frustum from_camera(const camera::CameraState& camera, const math::Vector3& position);
        CullTest test_bounds(const BoundingBox& box) const;
        CullTest test_sphere(const math::Vector3& center, float radius) const;
    };

    SceneCulling();

    void set_camera(const camera::CameraState& camera, const math::Vector3& position);
    void update_frustum();

    CullingResult cull_layers(const scene::EvaluatedCompositionState& state);
    CullingResult cull_layers_frustum(const scene::EvaluatedCompositionState& state, const Frustum& frustum);

    std::vector<LODResult> compute_lod(const scene::EvaluatedCompositionState& state, const math::Vector3& camera_position);

    void set_enable_frustum_culling(bool enable) { frustum_culling_enabled_ = enable; }
    void set_enable_occlusion_culling(bool enable) { occlusion_culling_enabled_ = enable; }
    void set_enable_lod(bool enable) { lod_enabled_ = enable; }
    void set_culling_distance(float distance) { culling_distance_ = distance; }
    void set_lod_distances(float near_dist, float mid_dist, float far_dist);

    bool is_frustum_culling_enabled() const { return frustum_culling_enabled_; }
    bool is_occlusion_culling_enabled() const { return occlusion_culling_enabled_; }
    bool is_lod_enabled() const { return lod_enabled_; }

    const Frustum& get_frustum() const { return frustum_; }
    const std::vector<BoundingBox>& get_layer_bounds() const { return layer_bounds_; }

    struct OcclusionQuery {
        std::size_t object_id{0};
        bool occlusion_test{false};
        bool visible_in_last_frame{false};
        int sample_count{0};
    };

    void submit_occlusion_query(std::size_t object_id, bool is_occluded);
    bool get_occlusion_result(std::size_t object_id) const;

    void set_quality_tier(const std::string& tier);
    std::string get_quality_tier() const { return quality_tier_; }

private:
    BoundingBox compute_layer_bounds(const scene::EvaluatedLayerState& layer);
    LODLevel compute_lod_level(float distance) const;

    bool frustum_culling_enabled_;
    bool occlusion_culling_enabled_;
    bool lod_enabled_;
    float culling_distance_;
    float lod_near_{50.0f};
    float lod_mid_{200.0f};
    float lod_far_{500.0f};

    camera::CameraState camera_;
    math::Vector3 camera_position_;
    Frustum frustum_;
    std::vector<BoundingBox> layer_bounds_;
    std::unordered_map<std::size_t, OcclusionQuery> occlusion_queries_;
    std::string quality_tier_;
};

inline SceneCulling::Frustum SceneCulling::Frustum::from_camera(const camera::CameraState& camera, const math::Vector3& position) {
    Frustum f;
    math::Vector3 forward = camera.transform.rotation.rotate_vector({0, 0, -1}).normalized();
    math::Vector3 right = camera.transform.rotation.rotate_vector({1, 0, 0}).normalized();
    math::Vector3 up = camera.transform.rotation.rotate_vector({0, 1, 0}).normalized();

    float fov_tan = std::tan(camera.fov_y_rad * 0.5f);
    float aspect = camera.aspect;

    math::Vector3 rightVec = right * fov_tan * aspect;
    math::Vector3 upVec = up * fov_tan;

    f.planes[0] = forward;
    f.planes[1] = -forward;
    f.planes[2] = math::Vector3::cross(rightVec + upVec, forward);
    f.planes[3] = math::Vector3::cross(forward, rightVec - upVec);
    f.planes[4] = math::Vector3::cross(upVec, forward);
    f.planes[5] = math::Vector3::cross(forward, upVec);

    f.corners[0] = position + forward + rightVec - upVec;
    f.corners[1] = position + forward - rightVec - upVec;
    f.corners[2] = position + forward + rightVec + upVec;
    f.corners[3] = position + forward - rightVec + upVec;
    f.corners[4] = position - forward + rightVec - upVec;
    f.corners[5] = position - forward - rightVec - upVec;
    f.corners[6] = position - forward + rightVec + upVec;
    f.corners[7] = position - forward - rightVec + upVec;

    return f;
}

inline SceneCulling::CullTest SceneCulling::Frustum::test_bounds(const BoundingBox& box) const {
    if (box.is_empty()) return CullTest::Outside;

    math::Vector3 box_center = box.center();
    math::Vector3 box_extents = box.extents() * 0.5f;

    for (int i = 0; i < 6; ++i) {
        const math::Vector3& normal = planes[i];
        float distance = math::Vector3::dot(normal, box_center) + math::Vector3::dot(box_extents, math::Vector3{std::abs(normal.x), std::abs(normal.y), std::abs(normal.z)});
        if (distance < 0.0f) return CullTest::Outside;
    }

    return CullTest::Inside;
}

inline SceneCulling::CullTest SceneCulling::Frustum::test_sphere(const math::Vector3& center, float radius) const {
    for (int i = 0; i < 6; ++i) {
        if (math::Vector3::dot(planes[i], center) + radius < 0.0f) return CullTest::Outside;
    }
    return CullTest::Inside;
}

} // namespace tachyon::renderer3d

