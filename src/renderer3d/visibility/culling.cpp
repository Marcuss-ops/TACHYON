#include "tachyon/renderer3d/visibility/culling.h"
#include "tachyon/media/loading/mesh_asset.h"
#include <algorithm>
#include <cmath>
#include <chrono>

namespace tachyon::renderer3d {

SceneCulling::SceneCulling()
    : frustum_culling_enabled_(true)
    , occlusion_culling_enabled_(false)
    , lod_enabled_(true)
    , culling_distance_(1000.0f)
    , quality_tier_("high") 
{
    layer_bounds_.reserve(64);
}

void SceneCulling::set_camera(const camera::CameraState& camera, const math::Vector3& position) {
    camera_ = camera;
    camera_position_ = position;
    update_frustum();
}

void SceneCulling::update_frustum() {
    frustum_ = Frustum::from_camera(camera_, camera_position_);
}

SceneCulling::CullingResult SceneCulling::cull_layers(const scene::EvaluatedCompositionState& state, const render::RenderIntent* intent) {
    CullingResult result;
    auto start = std::chrono::high_resolution_clock::now();

    if (!frustum_culling_enabled_) {
        result.visibleIndices.reserve(state.layers.size());
        for (std::size_t i = 0; i < state.layers.size(); ++i) {
            if (state.layers[i].is_3d && state.layers[i].visible) {
                result.visibleIndices.push_back(i);
            }
        }
        result.total_objects = state.layers.size();
        result.visible_count = result.visibleIndices.size();
        result.culled_count = result.total_objects - result.visible_count;
        return result;
    }

    if (layer_bounds_.size() != state.layers.size()) {
        layer_bounds_.resize(state.layers.size());
    }

    for (std::size_t i = 0; i < state.layers.size(); ++i) {
        const auto& layer = state.layers[i];
        if (!layer.is_3d || !layer.visible) {
            result.culledIndices.push_back(i);
        }

        layer_bounds_[i] = compute_layer_bounds(layer, intent);
        float dist = (layer.world_position3 - camera_position_).length();

        if (dist > culling_distance_ || frustum_.test_bounds(layer_bounds_[i]) == CullTest::Outside) {
            result.culledIndices.push_back(i);
            continue;
        }

        result.visibleIndices.push_back(i);
    }

    result.total_objects = state.layers.size();
    result.visible_count = result.visibleIndices.size();
    result.culled_count = result.culledIndices.size();

    auto end = std::chrono::high_resolution_clock::now();
    result.culling_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

    return result;
}

SceneCulling::CullingResult SceneCulling::cull_layers_frustum(const scene::EvaluatedCompositionState& state, const Frustum& frustum, const render::RenderIntent* intent) {
    CullingResult result;
    auto start = std::chrono::high_resolution_clock::now();

    if (layer_bounds_.size() != state.layers.size()) {
        layer_bounds_.resize(state.layers.size());
    }

    for (std::size_t i = 0; i < state.layers.size(); ++i) {
        const auto& layer = state.layers[i];
        if (!layer.is_3d || !layer.visible) {
            result.culledIndices.push_back(i);
            continue;
        }

        layer_bounds_[i] = compute_layer_bounds(layer, intent);

        if (frustum.test_bounds(layer_bounds_[i]) == CullTest::Outside) {
            result.culledIndices.push_back(i);
            continue;
        }

        result.visibleIndices.push_back(i);
    }

    result.total_objects = state.layers.size();
    result.visible_count = result.visibleIndices.size();
    result.culled_count = result.culledIndices.size();

    auto end = std::chrono::high_resolution_clock::now();
    result.culling_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

    return result;
}

std::vector<SceneCulling::LODResult> SceneCulling::compute_lod(const scene::EvaluatedCompositionState& state, const math::Vector3& camera_position) {
    std::vector<LODResult> results;

    if (!lod_enabled_) {
        results.reserve(state.layers.size());
        for (std::size_t i = 0; i < state.layers.size(); ++i) {
            results.push_back({i, LODLevel::High, 0.0f, false});
        }
        return results;
    }

    results.reserve(state.layers.size());

    for (std::size_t i = 0; i < state.layers.size(); ++i) {
        const auto& layer = state.layers[i];
        if (!layer.is_3d || !layer.visible) {
            results.push_back({i, LODLevel::High, 0.0f, false});
            continue;
        }

        float dist = (layer.world_position3 - camera_position).length();
        LODLevel level = compute_lod_level(dist);
        bool use_low = (level == LODLevel::Low || level == LODLevel::Medium);

        results.push_back({i, level, dist, use_low});
    }

    return results;
}

void SceneCulling::set_lod_distances(float near_dist, float mid_dist, float far_dist) {
    lod_near_ = near_dist;
    lod_mid_ = mid_dist;
    lod_far_ = far_dist;
}

void SceneCulling::submit_occlusion_query(std::size_t object_id, bool is_occluded) {
    auto it = occlusion_queries_.find(object_id);
    if (it == occlusion_queries_.end()) {
        occlusion_queries_[object_id] = {object_id, true, !is_occluded, 1};
    } else {
        it->second.visible_in_last_frame = !is_occluded;
        it->second.sample_count++;
    }
}

bool SceneCulling::get_occlusion_result(std::size_t object_id) const {
    auto it = occlusion_queries_.find(object_id);
    if (it == occlusion_queries_.end()) return true;
    return it->second.visible_in_last_frame;
}

void SceneCulling::set_quality_tier(const std::string& tier) {
    quality_tier_ = tier;
    if (tier == "low") {
        lod_enabled_ = false;
        frustum_culling_enabled_ = true;
        occlusion_culling_enabled_ = false;
        culling_distance_ = 500.0f;
    } else if (tier == "medium") {
        lod_enabled_ = true;
        frustum_culling_enabled_ = true;
        occlusion_culling_enabled_ = false;
        culling_distance_ = 1000.0f;
        set_lod_distances(30.0f, 150.0f, 400.0f);
    } else {
        lod_enabled_ = true;
        frustum_culling_enabled_ = true;
        occlusion_culling_enabled_ = true;
        culling_distance_ = 2000.0f;
        set_lod_distances(50.0f, 200.0f, 500.0f);
    }
}

SceneCulling::BoundingBox SceneCulling::compute_layer_bounds(const scene::EvaluatedLayerState& layer, const render::RenderIntent* intent) {
    const media::MeshAsset* mesh = nullptr;
    if (intent) {
        auto it = intent->layer_resources.find(layer.id);
        if (it != intent->layer_resources.end()) {
            mesh = it->second.mesh_asset.get();
        }
    }

    if (mesh && !mesh->sub_meshes.empty()) {
        const auto& sub = mesh->sub_meshes[0];
        if (!sub.vertices.empty()) {
            math::Vector3 min = sub.vertices[0].position;
            math::Vector3 max = sub.vertices[0].position;
            for (const auto& v : sub.vertices) {
                if (v.position.x < min.x) min.x = v.position.x;
                if (v.position.y < min.y) min.y = v.position.y;
                if (v.position.z < min.z) min.z = v.position.z;
                if (v.position.x > max.x) max.x = v.position.x;
                if (v.position.y > max.y) max.y = v.position.y;
                if (v.position.z > max.z) max.z = v.position.z;
            }
            math::Vector3 scaled_extents = (max - min) * 0.5f;
            scaled_extents = {
                scaled_extents.x * layer.scale_3d.x * 0.01f,
                scaled_extents.y * layer.scale_3d.y * 0.01f,
                scaled_extents.z * layer.scale_3d.z * 0.01f
            };
            return {layer.world_position3 - scaled_extents, layer.world_position3 + scaled_extents};
        }
    }

    return BoundingBox::from_center_extents(layer.world_position3, layer.scale_3d);
}

SceneCulling::LODLevel SceneCulling::compute_lod_level(float distance) const {
    if (distance < lod_near_) return LODLevel::Ultra;
    if (distance < lod_mid_) return LODLevel::High;
    if (distance < lod_far_) return LODLevel::Medium;
    return LODLevel::Low;
}

} // namespace tachyon::renderer3d
