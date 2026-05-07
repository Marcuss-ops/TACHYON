#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/objects/procedural_spec.h"
#include "tachyon/core/spec/schema/3d/three_d_spec.h"
#include "tachyon/core/scene/constraints/constraints.h"
#include "tachyon/core/math/matrix4x4.h"
#include "tachyon/core/math/transform2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/shapes/shape_path.h"
#include "tachyon/core/scene/state/evaluated_camera_state.h"
#include "tachyon/core/spec/schema/common/gradient_spec.h"
#include "tachyon/core/spec/schema/objects/mask_spec.h"
#include "tachyon/core/spec/schema/objects/path_spec.h"
#include "tachyon/text/content/word_timestamps.h"
#include <vector>
#include <string>
#include <memory>
#include <optional>


namespace tachyon::scene {

using LayerType = ::tachyon::LayerType;
using TrackMatteType = ::tachyon::TrackMatteType;

struct EvaluatedMaterialState {
    float metallic{0.0f};
    float roughness{0.5f};
    float ior{1.45f};
    float emission{0.0f};
    float subsurface{0.0f};
    float specular{0.5f};
    float specular_tint{0.0f};
    float anisotropic{0.0f};
    float sheen{0.0f};
    float sheen_tint{0.0f};
    float clearcoat{0.0f};
    float clearcoat_roughness{0.03f};
    float transmission{0.0f};
};

struct EvaluatedLightState {
    std::string id;
    std::string layer_id;
    std::string type;
    math::Vector3 position;
    math::Vector3 direction;
    math::Vector3 color;
    float intensity;
    float attenuation_near{0.0f};
    float attenuation_far{0.0f};
    float cone_angle{45.0f};
    float cone_feather{0.0f};
    bool casts_shadows{true};
    float shadow_darkness{0.0f};
    float shadow_radius{0.0f};
    std::string falloff_type{"inverse_square"};
    float range{0.0f};
};

struct EvaluatedShapePathPoint {
    math::Vector2 position;
    math::Vector2 tangent_in;
    math::Vector2 tangent_out;
};

struct EvaluatedShapePath {
    bool closed{false};
    std::vector<EvaluatedShapePathPoint> points;
};

struct EvaluatedCompositionState;

struct EvaluatedLayerState {
    std::string layer_id;
    std::string id;
    std::string name;
    std::size_t layer_index{0};
    LayerType type{LayerType::Solid};
    
    math::Matrix4x4 world_matrix{math::Matrix4x4::identity()};
    math::Vector3 world_position3{0.0f, 0.0f, 0.0f};
    math::Vector3 world_normal{0.0f, 0.0f, 1.0f};
    math::Vector3 scale_3d{1.0f, 1.0f, 1.0f};
    float extrusion_depth{0.0f};
    float bevel_size{0.0f};
    math::Transform2 local_transform{math::Transform2::identity()};
    bool motion_blur{false};
    
    bool visible{true};
    bool enabled{true};
    bool active{true};
    bool is_3d{false};
    bool is_adjustment_layer{false};
    float opacity{1.0f};
    std::string blend_mode{"normal"};

    // Mask feather for matte rendering
    float mask_feather{0.0f};
    
    double local_time_seconds{0.0};
    double child_time_seconds{0.0};
    int width{0};
    int height{0};
    ColorSpec fill_color{255, 255, 255, 255};
    ColorSpec stroke_color{255, 255, 255, 255};
    float stroke_width{0.0f};
    std::optional<ShapePathSpec> shape_path;
    std::optional<ShapeSpec> shape_spec;
    std::optional<spec::MaskPath> mask_path;
    std::vector<EffectSpec> effects;
    std::vector<TextAnimatorSpec> text_animators;
    std::vector<TextHighlightSpec> text_highlights;
    std::vector<ConstraintSpec> constraints;
    std::vector<IKSpec> ik_chains;
    std::optional<std::string> asset_path;
    std::optional<GradientSpec> gradient_fill;
    std::optional<GradientSpec> gradient_stroke;
    std::optional<std::string> subtitle_path;
    std::optional<std::string> word_timestamp_path;
    std::optional<ColorSpec> subtitle_outline_color;
    float subtitle_outline_width{0.0f};
    spec::LineCap line_cap{spec::LineCap::Butt};
    spec::LineJoin line_join{spec::LineJoin::Miter};
    float miter_limit{4.0f};
    int text_alignment{0};
    std::string text_content;
    std::string font_id;
    float font_size{0.0f};
    // Renderer-specific resource references (ID-based instead of pointers)
    std::optional<std::string> mesh_asset_id;
    std::optional<std::string> texture_asset_id;
    EvaluatedMaterialState material;
    
    // Animation/Rigging state
    std::vector<math::Matrix4x4> joint_matrices;
    std::vector<float> morph_weights;

    std::optional<std::size_t> track_matte_layer_index;
    TrackMatteType track_matte_type{TrackMatteType::None};
    std::optional<std::string> precomp_id;
    bool collapse_transformations{false}; // Merge precomp layers into parent comp when true
    
    std::shared_ptr<EvaluatedCompositionState> nested_composition;
    std::vector<math::Vector2> corner_pin;
    
    // Camera properties (if type == Camera)
    std::string camera_type{"one_node"};
    float zoom{877.0f};
    float camera_roll{0.0f};
    math::Vector3 poi{0.0f, 0.0f, 0.0f};
    
    // Camera Shake state
    uint64_t camera_shake_seed{0};
    float camera_shake_amplitude_pos{0.0f};
    float camera_shake_amplitude_rot{0.0f};
    float camera_shake_frequency{0.0f};
    float camera_shake_roughness{0.0f};

    // Camera Depth of Field
    std::optional<float> camera_aperture;
    std::optional<float> camera_focus_distance;

    // Previous state for motion blur calculation
    math::Matrix4x4 previous_world_matrix{math::Matrix4x4::identity()};

    // Masks
    std::vector<spec::MaskPath> masks;
    // Trim Paths support
    float trim_start{0.0f};
    float trim_end{1.0f};
    float trim_offset{0.0f};

    bool text_on_path_enabled{false};

    // Mesh deformation resource reference; empty means no deform is applied.
    std::optional<std::string> mesh_deform_id;

    // Effects
    std::vector<EffectSpec> animated_effects;

    // Transitions
    LayerTransitionSpec transition_in;
    LayerTransitionSpec transition_out;
    double in_time{0.0};
    double out_time{10.0};

    // Procedural generation
    std::optional<ProceduralSpec> procedural;
    
    // 3D modifiers
    std::optional<ThreeDSpec> three_d;
};

struct EvaluatedCompositionState {
    std::string composition_id;
    std::string composition_name;
    int width{1920};
    int height{1080};
    FrameRate frame_rate;
    std::int64_t frame_number{0};
    double composition_time_seconds{0.0};
    
    std::vector<EvaluatedLayerState> layers;
    std::vector<EvaluatedLightState> lights;
    EvaluatedCameraState camera;
    
    ColorSpec background_color{0, 0, 0, 0};

    // Neutral environment reference resolved later by renderers.
    std::optional<std::string> environment_map_id;
};

} // namespace tachyon::scene
