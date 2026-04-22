#pragma once

#include "tachyon/core/camera/camera_state.h"
#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/math/transform2.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/scene/constraints/constraints.h"
#include "tachyon/renderer2d/raster/path_rasterizer.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <memory>
#include <string>
#include <vector>

namespace tachyon { namespace media { struct HDRTextureData; struct MeshAsset; }
namespace scene {

enum class LayerType {
    Unknown,
    Solid,
    Shape,
    Mask,
    Image,
    Text,
    Camera,
    Precomp,
    Light,
    NullLayer,
    Video
};

struct EvaluatedCompositionState;

struct EvaluatedLightState {
    std::string layer_id;
    std::string type; // "ambient", "parallel", "point", "spot"
    math::Vector3 position{math::Vector3::zero()};
    math::Vector3 direction{0.0f, 0.0f, -1.0f};
    ColorSpec color{255, 255, 255, 255};
    float intensity{1.0f};
    
    // Attenuation (point + spot only)
    float attenuation_near{0.0f};   // distance where full intensity starts
    float attenuation_far{1000.0f}; // distance where intensity reaches 0
    
    // Spot cone (spot only)
    float cone_angle{90.0f};       // Full angle in degrees
    float cone_feather{50.0f};     // 0..100 percentage
    
    // Falloff shape
    std::string falloff_type{"smooth"};

    // Shadows
    bool casts_shadows{false};
    float shadow_darkness{1.0f}; // 0..1
    float shadow_radius{0.0f};   // for soft shadows
};

struct EvaluatedShapePathPoint {
    math::Vector2 position{math::Vector2::zero()};
    math::Vector2 tangent_in{math::Vector2::zero()};
    math::Vector2 tangent_out{math::Vector2::zero()};
};

struct EvaluatedShapePath {
    std::vector<EvaluatedShapePathPoint> points;
    bool closed{true};
};

struct EvaluatedLayerState {
    EvaluatedLayerState() = default;
    EvaluatedLayerState(const EvaluatedLayerState& other);
    EvaluatedLayerState& operator=(const EvaluatedLayerState& other);
    EvaluatedLayerState(EvaluatedLayerState&&) noexcept = default;
    EvaluatedLayerState& operator=(EvaluatedLayerState&&) noexcept = default;

    std::size_t layer_index{0};
    std::string id;
    std::string name;
    LayerType type{LayerType::Unknown};
    std::string blend_mode{"normal"};
    bool enabled{false};
    bool active{false};
    bool visible{false};
    bool is_3d{false};
    bool is_adjustment_layer{false};
    
    double local_time_seconds{0.0};
    double child_time_seconds{0.0};
    double opacity{1.0};
    
    math::Transform2 local_transform{math::Transform2::identity()};
    math::Matrix4x4 world_matrix{math::Matrix4x4::identity()};
    std::optional<math::Matrix4x4> previous_world_matrix;
    
    std::string parent_id;
    math::Vector3 local_position3{math::Vector3::zero()};
    math::Quaternion local_rotation3{math::Quaternion::identity()};
    math::Vector3 local_scale3{1.0f, 1.0f, 1.0f};

    math::Vector3 orientation_xyz_deg{math::Vector3::zero()};
    math::Vector3 anchor_point_3d{math::Vector3::zero()};
    math::Vector3 scale_3d{100.0f, 100.0f, 100.0f};
    
    math::Vector3 world_position3{math::Vector3::zero()};
    math::Vector3 world_normal{0.0f, 0.0f, 1.0f};

    struct EvaluatedMaterialState {
        float metallic{0.0f};
        float roughness{0.5f};
        float emission{0.0f};
        float ior{1.45f};
        
        // Disney BSDF Parameters
        float subsurface{0.0f};
        float specular{0.5f};
        float specular_tint{0.0f};
        float anisotropic{0.0f};
        float sheen{0.0f};
        float sheen_tint{0.5f};
        float clearcoat{0.0f};
        float clearcoat_roughness{0.03f};
        float transmission{0.0f};
    };
    EvaluatedMaterialState material;

    float extrusion_depth{0.0f};
    float bevel_size{0.0f};
    
    std::int64_t width{0};
    std::int64_t height{0};
    
    std::string text_content;
    std::string font_id;
    float font_size{48.0f};
    int text_alignment{0}; 
    ColorSpec fill_color{255, 255, 255, 255};
    ColorSpec stroke_color{255, 255, 255, 255};
    float stroke_width{0.0f};
    renderer2d::LineCap line_cap{renderer2d::LineCap::Butt};
    renderer2d::LineJoin line_join{renderer2d::LineJoin::Miter};
    float miter_limit{4.0f};
    
    std::optional<EvaluatedShapePath> shape_path;
    std::vector<EffectSpec> effects;
    std::vector<TextAnimatorSpec> text_animators;
    std::vector<TextHighlightSpec> text_highlights;
    float mask_feather{0.0f};
    std::optional<std::string> subtitle_path;
    std::optional<ColorSpec> subtitle_outline_color;
    float subtitle_outline_width{0.0f};
    
    std::optional<GradientSpec> gradient_fill;
    std::optional<GradientSpec> gradient_stroke;

    float trim_start{0.0f};
    float trim_end{1.0f};
    float trim_offset{0.0f};
    
    TrackMatteType track_matte_type{TrackMatteType::None};
    std::optional<std::size_t> track_matte_layer_index;

    std::optional<std::string> precomp_id;
    std::unique_ptr<EvaluatedCompositionState> nested_composition;

    std::optional<std::string> asset_id;
    std::optional<std::string> asset_path;

    media::MeshAsset* mesh_asset{nullptr};
    const std::uint8_t* texture_rgba{nullptr};

    std::vector<float> morph_weights;
    std::vector<math::Matrix4x4> joint_matrices;
    
    std::vector<ConstraintSpec> constraints;
    std::vector<IKSpec> ik_chains;
    TrackingSpec tracking;
};

struct EvaluatedCameraState {
    bool available{false};
    std::string layer_id;
    math::Vector3 position{math::Vector3::zero()};
    math::Vector3 point_of_interest{0.0f, 0.0f, 0.0f}; 
    std::optional<math::Vector3> previous_position;
    std::optional<math::Vector3> previous_point_of_interest;
    bool is_two_node{true};

    bool  dof_enabled{false};
    float focus_distance{1000.0f};
    float aperture{1.0f};
    float blur_level{1.0f};
    int   aperture_blades{0};
    float aperture_rotation{0.0f};
    float shutter_angle{180.0f};

    camera::CameraState camera;
};

struct EvaluatedCompositionState {
    std::string composition_id;
    std::string composition_name;
    std::int64_t width{0};
    std::int64_t height{0};
    FrameRate frame_rate;
    std::int64_t frame_number{0};
    double composition_time_seconds{0.0};
    std::vector<EvaluatedLayerState> layers;
    const media::HDRTextureData* environment_map{nullptr};
    float environment_intensity{1.0f};
    float environment_intensity_rotation{0.0f};
    std::vector<EvaluatedLightState> lights;
    EvaluatedCameraState camera;
};

} // namespace scene
} // namespace tachyon
