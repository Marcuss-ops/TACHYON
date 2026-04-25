#pragma once

#include "tachyon/core/math/vector3.h"
#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/spec/schema/common/common_spec.h"

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace tachyon::media {
struct MeshAsset;
}

namespace tachyon::renderer3d {

enum class LightType {
    Ambient,
    Directional,
    Point,
    Spot,
    Area
};

struct EvaluatedLight {
    LightType type;
    ::tachyon::math::Vector3 position;
    ::tachyon::math::Vector3 direction; // For directional/spot
    ::tachyon::ColorSpec color;
    float intensity{1.0f};
    float attenuation_radius{0.0f}; // 0 = infinite
    float spot_angle{45.0f};
    float spot_penumbra{0.0f};
    bool casts_shadows{true};
};

struct EvaluatedMaterial {
    ::tachyon::ColorSpec base_color{255, 255, 255, 255};
    float opacity{1.0f};
    float metallic{0.0f};
    float roughness{0.5f};
    float emission_strength{0.0f};
    ::tachyon::ColorSpec emission_color{0, 0, 0, 255};
    float transmission{0.0f}; // Glass/refraction
    float ior{1.45f};         // Index of refraction
};

struct EvaluatedMeshInstance {
    std::uint32_t object_id;
    std::uint32_t material_id;
    ::tachyon::math::Matrix4x4 world_transform;
    std::optional<::tachyon::math::Matrix4x4> previous_world_transform;
    EvaluatedMaterial material;
    
    // Geometry reference (would point to an asset/VBO)
    std::string mesh_asset_id;
    std::shared_ptr<const ::tachyon::media::MeshAsset> mesh_asset;
};

struct EvaluatedCamera3D {
    ::tachyon::math::Vector3 position;
    ::tachyon::math::Vector3 target;
    ::tachyon::math::Vector3 up{0.0f, 1.0f, 0.0f};
    
    std::optional<::tachyon::math::Vector3> previous_position;
    std::optional<::tachyon::math::Vector3> previous_target;
    std::optional<::tachyon::math::Vector3> previous_up;
    
    float fov_y{45.0f};
    float focal_length_mm{35.0f}; // Physical focal length
    float focal_distance{100.0f}; // For Depth of Field
    float aperture{0.0f};         // 0 = Pinhole camera (no DoF)
    
    // Camera Shake (Phase 4)
    bool shake_enabled{false};
    float shake_amplitude{0.0f};   // World units
    float shake_frequency{1.0f};   // Hz
    std::uint32_t shake_seed{0};   // Deterministic seed
    
    // Multi-Camera Cuts (Phase 5)
    std::string camera_id{""};
    bool is_active_camera{true};
};

/**
 * @brief The immutable, resolved representation of a 3D scene at time `t`.
 * This serves as the strict contract between the evaluation engine and the RayTracer.
 */
struct EvaluatedScene3D {
    EvaluatedCamera3D camera;
    std::vector<EvaluatedLight> lights;
    std::vector<EvaluatedMeshInstance> instances;
    
    // Environment
    std::string environment_map_id;
    float environment_intensity{1.0f};
    float environment_rotation{0.0f};
};

} // namespace tachyon::renderer3d
