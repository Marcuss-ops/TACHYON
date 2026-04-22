#pragma once

#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/core/spec/schema/properties/property_spec.h"
#include "tachyon/core/spec/schema/transform/transform_spec.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/renderer2d/raster/path_rasterizer.h"
#include "tachyon/core/scene/constraints/constraints.h"
#include <string>
#include <vector>
#include <optional>

namespace tachyon {

struct ShapePathPointSpec {
    math::Vector2 position{math::Vector2::zero()};
    math::Vector2 tangent_in{math::Vector2::zero()};
    math::Vector2 tangent_out{math::Vector2::zero()};
};

struct ShapePathSpec {
    std::vector<ShapePathPointSpec> points;
    bool closed{true};
};

struct TrackingSpec {
    std::string mode{"none"}; // "none", "point", "planar"
    std::string source_layer_id;
    std::vector<math::Vector2> points;
    std::vector<float> transform_matrix; // 3x3 for homography or 2x3 for affine
};

struct EffectSpec {
    std::string type;
    std::string id;
    bool enabled{true};
    std::unordered_map<std::string, double> scalars;
    std::unordered_map<std::string, ColorSpec> colors;
    std::unordered_map<std::string, std::string> strings;
};

struct LayerSpec {
    std::string id;
    std::string type;
    std::string name;
    std::string blend_mode{"normal"};
    bool enabled{true};
    double start_time{0.0};
    double in_point{0.0};
    double out_point{0.0};
    double opacity{1.0};
    std::int64_t width{0};
    std::int64_t height{0};
    float stroke_width{0.0f};
    std::string text_content;
    std::string font_id;
    AnimatedScalarSpec font_size; // pixel size
    std::string alignment{"left"}; // left, center, right
    AnimatedColorSpec fill_color;
    AnimatedColorSpec stroke_color;
    AnimatedScalarSpec stroke_width_property;
    renderer2d::LineCap line_cap{renderer2d::LineCap::Butt};
    renderer2d::LineJoin line_join{renderer2d::LineJoin::Miter};
    float miter_limit{4.0f};
    std::optional<std::string> parent;
    bool is_3d{false};
    bool visible{true};
    bool is_adjustment_layer{false};
    bool motion_blur{false};
    AnimatedScalarSpec repeater_stagger_delay;
    std::optional<std::string> motion_path_id;
    AnimatedScalarSpec motion_path_progress;
    Transform2D transform;
    Transform3D transform3d;
    std::optional<ShapePathSpec> shape_path;
    std::vector<ShapePathSpec> morph_targets;
    AnimatedScalarSpec morph_weight; // 0..100 percentage
    std::vector<EffectSpec> effects;
    AnimatedScalarSpec opacity_property;
    AnimatedScalarSpec time_remap_property;
    TrackMatteType track_matte_type{TrackMatteType::None};
    std::optional<std::string> track_matte_layer_id;
    std::optional<std::string> precomp_id;
    std::optional<std::string> mesh_path;
    
    AnimatedScalarSpec trim_start;
    AnimatedScalarSpec trim_end;
    AnimatedScalarSpec trim_offset;
    
    AnimatedScalarSpec repeater_count;
    AnimatedScalarSpec repeater_offset_position_x;
    AnimatedScalarSpec repeater_offset_position_y;
    AnimatedScalarSpec repeater_offset_rotation;
    AnimatedScalarSpec repeater_offset_scale_x;
    AnimatedScalarSpec repeater_offset_scale_y;
    AnimatedScalarSpec repeater_start_opacity;
    AnimatedScalarSpec repeater_end_opacity;

    std::optional<GradientSpec> gradient_fill;
    std::optional<GradientSpec> gradient_stroke;
    
    std::vector<TextAnimatorSpec> text_animators;
    std::vector<TextHighlightSpec> text_highlights;
    AnimatedScalarSpec mask_feather;

    std::optional<std::string> subtitle_path;
    std::optional<ColorSpec>   subtitle_outline_color;
    float                      subtitle_outline_width{0.0f};

    AnimatedScalarSpec ambient_coeff;
    AnimatedScalarSpec diffuse_coeff;
    AnimatedScalarSpec specular_coeff;
    AnimatedScalarSpec shininess;
    AnimatedScalarSpec metallic;
    AnimatedScalarSpec roughness;
    AnimatedScalarSpec emission;
    AnimatedScalarSpec ior;
    bool metal{false};

    AnimatedScalarSpec extrusion_depth;
    AnimatedScalarSpec bevel_size;

    bool               is_two_node{true};
    AnimatedVector3Spec point_of_interest;
    bool               dof_enabled{false};
    AnimatedScalarSpec focus_distance;
    AnimatedScalarSpec aperture;
    AnimatedScalarSpec blur_level;
    AnimatedScalarSpec aperture_blades;
    AnimatedScalarSpec aperture_rotation;

    std::optional<std::string> light_type;
    AnimatedScalarSpec intensity;
    AnimatedScalarSpec attenuation_near;
    AnimatedScalarSpec attenuation_far;
    AnimatedScalarSpec cone_angle;
    AnimatedScalarSpec cone_feather;
    std::string        falloff_type{"smooth"};
    bool               casts_shadows{false};
    AnimatedScalarSpec shadow_darkness;
    AnimatedScalarSpec shadow_radius;
    
    std::vector<scene::ConstraintSpec> constraints;
    std::vector<scene::IKSpec> ik_chains;
    TrackingSpec tracking;
};

} // namespace tachyon
