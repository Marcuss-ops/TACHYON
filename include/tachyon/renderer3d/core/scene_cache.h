#pragma once

#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/camera/camera_state.h"
#include "tachyon/core/math/transform3.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/math/matrix4x4.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include <unordered_map>

namespace tachyon::renderer3d {

class SceneCache {
public:
    enum class DirtyFlag {
        Clean,
        GeometryOnly,
        TransformOnly,
        LightsOnly,
        CameraOnly,
        All
    };

    struct HashState {
        std::uint64_t mesh_hash{0};
        std::uint64_t transform_hash{0};
        std::uint64_t light_hash{0};
        std::uint64_t camera_hash{0};
        std::uint64_t material_hash{0};
        std::uint64_t content_hash{0};

        bool is_dirty() const { return content_hash == 0 && (mesh_hash != 0 || transform_hash != 0 || light_hash != 0 || camera_hash != 0); }
        bool needs_full_rebuild() const { return mesh_hash != 0 || transform_hash != 0; }
    };

    struct LayerCacheEntry {
        std::uint64_t geometry_hash{0};
        std::uint64_t transform_hash{0};
        std::uint64_t material_hash{0};
        std::uint64_t bounds_hash{0};
        math::Vector3 bounds_min{};
        math::Vector3 bounds_max{};
        bool visible{true};
    };

    struct LightCacheEntry {
        std::uint64_t properties_hash{0};
        math::Vector3 last_position{};
        math::Vector3 last_direction{};
        float last_intensity{0.0f};
    };

    struct CameraCacheEntry {
        std::uint64_t properties_hash{0};
        math::Transform3 last_transform{};
        float last_fov{60.0f};
        float last_aperture{0.0f};
        float last_focus_distance{1000.0f};
        math::Vector3 last_position{};
    };

    SceneCache();

    void set_frame(std::int64_t frame_number, double time_seconds);
    void update_composition(const scene::EvaluatedCompositionState& state);

    bool needs_rebuild() const { return dirty_ != DirtyFlag::Clean; }
    DirtyFlag get_dirty_state() const { return dirty_; }
    const HashState& get_hash_state() const { return hash_state_; }

    const std::vector<LayerCacheEntry>& get_layer_cache() const { return layer_cache_; }
    const CameraCacheEntry& get_camera_cache() const { return camera_cache_; }
    const std::vector<LightCacheEntry>& get_light_cache() const { return light_cache_; }

    void force_rebuild();
    void mark_clean();

private:
    std::uint64_t compute_layer_hash(const scene::EvaluatedLayerState& layer, std::size_t index);
    std::uint64_t compute_light_hash(const scene::EvaluatedLightState& light, std::size_t index);
    std::uint64_t compute_camera_hash(const scene::EvaluatedCameraState& camera);
    std::uint64_t combine_hash(std::uint64_t a, std::uint64_t b) const;

    DirtyFlag check_dirty_flags(const scene::EvaluatedCompositionState& state);

    DirtyFlag dirty_;
    HashState hash_state_;

    std::int64_t last_frame_{0};
    double last_time_{0.0};

    std::vector<LayerCacheEntry> layer_cache_;
    std::vector<LightCacheEntry> light_cache_;
    CameraCacheEntry camera_cache_;

    math::Vector3 composition_bounds_min_;
    math::Vector3 composition_bounds_max_;
};

} // namespace tachyon::renderer3d
