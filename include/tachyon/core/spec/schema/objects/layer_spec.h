#pragma once

#include "tachyon/core/spec/schema/objects/procedural_spec.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"

#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/core/spec/schema/3d/three_d_spec.h"
#include "tachyon/core/spec/schema/properties/property_spec.h"
#include "tachyon/core/spec/schema/transform/transform_spec.h"
#include "tachyon/core/spec/schema/contracts/shared_contracts.h"
#include "tachyon/core/spec/schema/objects/mask_spec.h"
#include "tachyon/core/shapes/shape_path.h"
#include "tachyon/core/spec/schema/shapes/shape_spec.h"
#include "tachyon/core/animation/easing.h"
#include <map>
#include <string>
#include <vector>
#include <optional>

namespace tachyon {

struct ParticleSpec {
    std::string type;
    bool enabled{true};

    [[nodiscard]] bool empty() const noexcept {
        return type.empty() || !enabled;
    }
};

struct LayerTransitionSpec {
    std::string type{"none"};
    std::string transition_id;
    TransitionKind kind{TransitionKind::None}; 
    std::string direction; 
    double duration{0.4};
    double delay{0.0};
    animation::EasingPreset easing{animation::EasingPreset::EaseOut};
    struct SpringSpec {
        double stiffness{200.0};
        double damping{20.0};
        double mass{1.0};
        double velocity{0.0};
    } spring;
};

struct LayerSpec {
    std::string id;
    std::string name;
    std::string asset_id; // For image/video layers
    std::optional<std::string> mesh_deform_id; // Explicit deform resource reference
    std::string preset_id; // For procedural/preset layers
    LayerType type{LayerType::NullLayer}; // Canonical Source of Truth
    std::string type_string; // Legacy / Authoring fallback
    std::string blend_mode{"normal"};
    
    bool enabled{true};
    bool visible{true};
    bool is_3d{false};
    bool is_adjustment_layer{false};
    bool motion_blur{false};

    double start_time{0.0};
    double in_point{0.0};
    double out_point{10.0};
    double opacity{1.0};

    int width{1920};
    int height{1080};

    Transform2D transform;
    Transform3D transform3d;
    
    // 3D mesh generation
    std::optional<double> extrude; // Extrude depth for text layers
    std::optional<double> bevel;   // Bevel size for text layers
    
    AnimatedScalarSpec opacity_property;
    AnimatedScalarSpec mask_feather;
    AnimatedScalarSpec time_remap_property;

    std::optional<std::string> parent;
    std::optional<std::string> track_matte_layer_id;
    TrackMatteType track_matte_type{TrackMatteType::None};
    std::optional<std::string> precomp_id;

    // Text specific
    std::string text_content;
    std::string font_id;
    AnimatedScalarSpec font_size;
    std::string alignment{"left"};
    std::map<std::string, AnimatedScalarSpec> font_axes;
    AnimatedColorSpec fill_color;
    AnimatedColorSpec stroke_color;
    double stroke_width{0.0};
    AnimatedScalarSpec stroke_width_property;
    AnimatedScalarSpec metallic;
    AnimatedScalarSpec roughness;
    AnimatedScalarSpec ior;
    AnimatedScalarSpec transmission;
    AnimatedScalarSpec emission_strength;
    AnimatedColorSpec emission_color;

    // Repeater (from evaluator_composition.cpp)
    std::string repeater_type{"linear"};
    AnimatedScalarSpec repeater_count;
    AnimatedScalarSpec repeater_stagger_delay;
    AnimatedScalarSpec repeater_offset_position_x;
    AnimatedScalarSpec repeater_offset_position_y;
    AnimatedScalarSpec repeater_offset_rotation;
    AnimatedScalarSpec repeater_offset_scale_x;
    AnimatedScalarSpec repeater_offset_scale_y;
    AnimatedScalarSpec repeater_start_opacity;
    AnimatedScalarSpec repeater_end_opacity;
    AnimatedScalarSpec repeater_grid_cols;
    AnimatedScalarSpec repeater_radial_radius;
    AnimatedScalarSpec repeater_radial_start_angle;
    AnimatedScalarSpec repeater_radial_end_angle;

    // Subtitles
    std::string subtitle_path;
    AnimatedColorSpec subtitle_outline_color;
    double subtitle_outline_width{2.0};

    // Word timestamps for text highlight animation (word_highlight mode)
    std::string word_timestamp_path;

    // Vector Graphics
    std::optional<ShapePathSpec> shape_path;
    std::optional<ShapeSpec> shape_spec;
    std::string line_cap{"butt"};
    std::string line_join{"miter"};
    double miter_limit{4.0};
    
    // Camera specific
    std::string camera_type{"one_node"}; // "one_node" | "two_node"
    AnimatedScalarSpec camera_zoom;
    AnimatedVector3Spec camera_poi;
    AnimatedScalarSpec camera_aperture;
    AnimatedScalarSpec camera_focus_distance;

    // 2D Camera integration
    bool has_parallax{true};
    float parallax_factor{1.0f};
    std::optional<std::string> camera2d_id;

    // Camera Shake
    uint64_t camera_shake_seed{0};
    AnimatedScalarSpec camera_shake_amplitude_pos;
    AnimatedScalarSpec camera_shake_amplitude_rot;
    AnimatedScalarSpec camera_shake_frequency;
    AnimatedScalarSpec camera_shake_roughness;

    // Light specific
    std::string light_type{"point"}; // "point", "spot", "ambient", "parallel"
    AnimatedColorSpec light_color;
    AnimatedScalarSpec light_intensity;
    AnimatedScalarSpec attenuation_near;
    AnimatedScalarSpec attenuation_far;
    AnimatedScalarSpec cone_angle;
    AnimatedScalarSpec cone_feather;
    std::string falloff_type{"inverse_square"};
    bool casts_shadows{true};
    AnimatedScalarSpec shadow_darkness;
    AnimatedScalarSpec shadow_radius;

    // Effects & Animators (Skeletons)
    std::vector<EffectSpec> effects;  // Backward compatible static effects
    std::vector<AnimatedEffectSpec> animated_effects;  // Keyframeable effects
    std::vector<TextAnimatorSpec> text_animators;
    std::vector<TextHighlightSpec> text_highlights;

    // Mask paths (roto / vector masks)
    std::vector<spec::MaskPath> mask_paths;

    // Temporal & Tracking (Unified)
    std::vector<spec::TrackBinding> track_bindings;
    spec::TimeRemapCurve time_remap;
    spec::FrameBlendMode frame_blend{spec::FrameBlendMode::Linear};

    // Timing shorthand
    std::optional<double> duration;

    // Animation presets (inject transform/opacity keyframes via PresetCompiler)
    // Modern animation preset names
    std::string animation_in_preset;
    std::string animation_during_preset;
    std::string animation_out_preset;
    float animation_in_duration{0.4f};
    float animation_out_duration{0.4f};

    // Typed transitions
    LayerTransitionSpec transition_in;
    LayerTransitionSpec transition_out;

    // Playback behavior
    bool loop{false};
    bool hold_last_frame{false};

    // Markers
    struct MarkerSpec {
        double time{0.0};
        std::string label;
        std::string color{"#ffffff"};
    };
    std::vector<MarkerSpec> markers;

    // Procedural generation
    std::optional<ProceduralSpec> procedural;
    std::optional<ParticleSpec> particle_spec;
    
    std::optional<ThreeDSpec> three_d;
};

} // namespace tachyon
