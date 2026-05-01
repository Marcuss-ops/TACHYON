#include "tachyon/renderer3d/core/scene_cache.h"
#include "tachyon/media/loading/mesh_asset.h"

#include <cstring>
#include <limits>

namespace tachyon::renderer3d {

static std::uint64_t mix_hash(std::uint64_t h, std::uint64_t v) {
    h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    return h;
}

static std::uint64_t hash_float(float f) {
    std::uint32_t bits;
    std::memcpy(&bits, &f, sizeof(float));
    return static_cast<std::uint64_t>(bits);
}

static std::uint64_t hash_vector3(const math::Vector3& v) {
    std::uint64_t h = 1469598103934665603ULL;
    h = mix_hash(h, hash_float(v.x));
    h = mix_hash(h, hash_float(v.y));
    h = mix_hash(h, hash_float(v.z));
    return h;
}

static std::uint64_t hash_matrix4x4(const math::Matrix4x4& m) {
    std::uint64_t h = 1469598103934665603ULL;
    for (int i = 0; i < 16; ++i) {
        h = mix_hash(h, hash_float(m[i]));
    }
    return h;
}

SceneCache::SceneCache() : dirty_(DirtyFlag::All), last_frame_(0), last_time_(0.0) {
    composition_bounds_min_ = math::Vector3::zero();
    composition_bounds_max_ = math::Vector3::zero();
}

void SceneCache::set_frame(std::int64_t frame_number, double time_seconds) {
    if (frame_number != last_frame_ || time_seconds != last_time_) {
        last_frame_ = frame_number;
        last_time_ = time_seconds;
    }
}

void SceneCache::update_composition(const scene::EvaluatedCompositionState& state) {
    const DirtyFlag prev_dirty = dirty_;
    dirty_ = check_dirty_flags(state);

    hash_state_.content_hash = mix_hash(hash_state_.content_hash, static_cast<std::uint64_t>(state.frame_number));
    hash_state_.content_hash = mix_hash(hash_state_.content_hash, static_cast<std::uint64_t>(state.composition_time_seconds * 1000.0));

    if (state.layers.size() != layer_cache_.size()) {
        layer_cache_.resize(state.layers.size());
        dirty_ = DirtyFlag::All;
    }

    bool needs_transform = (dirty_ == DirtyFlag::TransformOnly || dirty_ == DirtyFlag::All);

    hash_state_.mesh_hash = 0;
    hash_state_.transform_hash = 0;

    composition_bounds_min_ = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
    composition_bounds_max_ = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };

    for (std::size_t i = 0; i < state.layers.size(); ++i) {
        const auto& layer = state.layers[i];
        auto& cache = layer_cache_[i];

        std::uint64_t new_geo_hash = compute_layer_hash(layer, i);
        std::uint64_t new_trans_hash = hash_matrix4x4(layer.world_matrix);

        if (cache.geometry_hash != new_geo_hash) {
            cache.geometry_hash = new_geo_hash;
            dirty_ = DirtyFlag::All;
        }
        if (cache.transform_hash != new_trans_hash) {
            cache.transform_hash = new_trans_hash;
            needs_transform = true;
        }

        hash_state_.mesh_hash = mix_hash(hash_state_.mesh_hash, cache.geometry_hash);
        hash_state_.transform_hash = mix_hash(hash_state_.transform_hash, cache.transform_hash);

        cache.bounds_min = layer.world_position3 - layer.scale_3d * 0.5f;
        cache.bounds_max = layer.world_position3 + layer.scale_3d * 0.5f;
        cache.visible = layer.visible && layer.is_3d;

        if (i == 0 || cache.bounds_min.x < composition_bounds_min_.x) composition_bounds_min_.x = cache.bounds_min.x;
        if (i == 0 || cache.bounds_min.y < composition_bounds_min_.y) composition_bounds_min_.y = cache.bounds_min.y;
        if (i == 0 || cache.bounds_min.z < composition_bounds_min_.z) composition_bounds_min_.z = cache.bounds_min.z;
        if (i == 0 || cache.bounds_max.x > composition_bounds_max_.x) composition_bounds_max_.x = cache.bounds_max.x;
        if (i == 0 || cache.bounds_max.y > composition_bounds_max_.y) composition_bounds_max_.y = cache.bounds_max.y;
        if (i == 0 || cache.bounds_max.z > composition_bounds_max_.z) composition_bounds_max_.z = cache.bounds_max.z;
    }

    if (state.lights.size() != light_cache_.size()) {
        light_cache_.resize(state.lights.size());
        dirty_ = DirtyFlag::LightsOnly;
    }

    hash_state_.light_hash = 0;
    for (std::size_t i = 0; i < state.lights.size(); ++i) {
        const auto& light = state.lights[i];
        auto& cache = light_cache_[i];

        std::uint64_t new_hash = compute_light_hash(light, i);
        if (cache.properties_hash != new_hash) {
            cache.properties_hash = new_hash;
            cache.last_position = light.position;
            cache.last_direction = light.direction;
            cache.last_intensity = light.intensity;

            if (dirty_ == DirtyFlag::Clean) dirty_ = DirtyFlag::LightsOnly;
        }

        hash_state_.light_hash = mix_hash(hash_state_.light_hash, cache.properties_hash);
    }

    hash_state_.camera_hash = compute_camera_hash(state.camera);

    if (state.camera.available) {
        camera_cache_.properties_hash = hash_state_.camera_hash;
        camera_cache_.last_transform = state.camera.camera.transform;
        camera_cache_.last_fov = state.camera.camera.fov_y_rad;
        camera_cache_.last_aperture = state.camera.aperture;
        camera_cache_.last_focus_distance = state.camera.focus_distance;
        camera_cache_.last_position = state.camera.position;
    }

    if (dirty_ == DirtyFlag::Clean && prev_dirty != DirtyFlag::Clean) {
    }
}

std::uint64_t SceneCache::compute_layer_hash(const scene::EvaluatedLayerState& layer, std::size_t index) {
    std::uint64_t h = static_cast<std::uint64_t>(index);
    h = mix_hash(h, static_cast<std::uint64_t>(layer.type));
    h = mix_hash(h, static_cast<std::uint64_t>(layer.layer_index));

    if (layer.mesh_asset) {
        const auto& mesh = layer.mesh_asset;
        h = mix_hash(h, static_cast<std::uint64_t>(mesh->sub_meshes.size()));
        if (!mesh->sub_meshes.empty()) {
            h = mix_hash(h, static_cast<std::uint64_t>(mesh->sub_meshes.front().vertices.size()));
            h = mix_hash(h, static_cast<std::uint64_t>(mesh->sub_meshes.front().indices.size()));
        }
    }

    h = mix_hash(h, static_cast<std::uint64_t>(layer.width));
    h = mix_hash(h, static_cast<std::uint64_t>(layer.height));
    h = mix_hash(h, static_cast<std::uint64_t>(layer.extrusion_depth > 0.0f ? 1 : 0));
    h = mix_hash(h, static_cast<std::uint64_t>(layer.bevel_size > 0.0f ? 1 : 0));

    h = mix_hash(h, hash_float(layer.material.metallic));
    h = mix_hash(h, hash_float(layer.material.roughness));
    h = mix_hash(h, hash_float(layer.material.emission));
    h = mix_hash(h, hash_float(layer.material.ior));

    return h;
}

std::uint64_t SceneCache::compute_light_hash(const scene::EvaluatedLightState& light, std::size_t index) {
    std::uint64_t h = static_cast<std::uint64_t>(index);

    if (light.type == "ambient") h = mix_hash(h, 1);
    else if (light.type == "parallel") h = mix_hash(h, 2);
    else if (light.type == "point") h = mix_hash(h, 3);
    else if (light.type == "spot") h = mix_hash(h, 4);

    h = mix_hash(h, hash_vector3(light.position));
    h = mix_hash(h, hash_vector3(light.direction));
    h = mix_hash(h, hash_float(light.intensity));
    h = mix_hash(h, hash_float(light.attenuation_near));
    h = mix_hash(h, hash_float(light.attenuation_far));

    return h;
}

std::uint64_t SceneCache::compute_camera_hash(const scene::EvaluatedCameraState& camera) {
    if (!camera.available) return 0;

    std::uint64_t h = 1469598103934665603ULL;
    h = mix_hash(h, hash_vector3(camera.position));
    h = mix_hash(h, hash_vector3(camera.point_of_interest));
    h = mix_hash(h, hash_float(camera.camera.fov_y_rad));
    h = mix_hash(h, hash_float(camera.aperture));
    h = mix_hash(h, hash_float(camera.focus_distance));
    h = mix_hash(h, hash_float(camera.camera.transform.rotation.x));
    h = mix_hash(h, hash_float(camera.camera.transform.rotation.y));
    h = mix_hash(h, hash_float(camera.camera.transform.rotation.z));
    h = mix_hash(h, hash_float(camera.camera.transform.rotation.w));

    return h;
}

std::uint64_t SceneCache::combine_hash(std::uint64_t a, std::uint64_t b) const {
    return mix_hash(a, b);
}

SceneCache::DirtyFlag SceneCache::check_dirty_flags(const scene::EvaluatedCompositionState& state) {
    if (layer_cache_.empty() || last_frame_ != state.frame_number) {
        return DirtyFlag::All;
    }

    bool geometry_changed = false;
    bool transform_changed = false;
    bool lights_changed = false;
    bool camera_changed = false;

    for (std::size_t i = 0; i < state.layers.size() && i < layer_cache_.size(); ++i) {
        const auto& layer = state.layers[i];
        const auto& cache = layer_cache_[i];

        if (cache.geometry_hash != compute_layer_hash(layer, i)) {
            geometry_changed = true;
        }
        if (cache.transform_hash != hash_matrix4x4(layer.world_matrix)) {
            transform_changed = true;
        }
    }

    for (std::size_t i = 0; i < state.lights.size() && i < light_cache_.size(); ++i) {
        if (light_cache_[i].properties_hash != compute_light_hash(state.lights[i], i)) {
            lights_changed = true;
        }
    }

    if (camera_cache_.properties_hash != compute_camera_hash(state.camera)) {
        camera_changed = true;
    }

    if (geometry_changed) return DirtyFlag::GeometryOnly;
    if (transform_changed) return DirtyFlag::TransformOnly;
    if (lights_changed) return DirtyFlag::LightsOnly;
    if (camera_changed) return DirtyFlag::CameraOnly;

    return DirtyFlag::Clean;
}

void SceneCache::force_rebuild() {
    dirty_ = DirtyFlag::All;
    hash_state_ = {};
    for (auto& cache : layer_cache_) {
        cache.geometry_hash = 0;
        cache.transform_hash = 0;
        cache.material_hash = 0;
        cache.bounds_hash = 0;
    }
    for (auto& cache : light_cache_) {
        cache.properties_hash = 0;
    }
    camera_cache_.properties_hash = 0;
}

void SceneCache::mark_clean() {
    dirty_ = DirtyFlag::Clean;
}

} // namespace tachyon::renderer3d
