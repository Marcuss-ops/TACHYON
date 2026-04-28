#pragma once

#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/core/spec/schema/properties/property_spec.h"
#include "tachyon/core/spec/schema/transform/transform_spec.h"
#include "tachyon/core/spec/schema/contracts/shared_contracts.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/renderer2d/path/mask_path.h"
#include "tachyon/core/shapes/shape_path.h"
#include "tachyon/core/spec/schema/shapes/shape_spec.h"
#include "tachyon/renderer2d/spec/gradient_spec.h"
#include <string>
#include <vector>
#include <optional>

namespace tachyon {

struct ProceduralSpec {
    std::string kind{"noise"}; 
    uint64_t seed{0};

    // Color Palette
    AnimatedColorSpec color_a{ColorSpec{255, 0, 0, 255}};
    AnimatedColorSpec color_b{ColorSpec{0, 0, 255, 255}};
    AnimatedColorSpec color_c{ColorSpec{0, 0, 0, 0}}; // Optional third color
    
    // Core Parameters
    AnimatedScalarSpec frequency{1.0};
    AnimatedScalarSpec speed{1.0};
    AnimatedScalarSpec amplitude{1.0};
    AnimatedScalarSpec scale{1.0};
    AnimatedScalarSpec angle{0.0};
    
    // Geometry & Grid Logic
    std::string shape{"square"}; // square, circle
    AnimatedScalarSpec spacing{50.0};
    AnimatedScalarSpec border_width{1.0};
    AnimatedColorSpec border_color{ColorSpec{150, 150, 150, 255}};
    
    // Warp / Distortion Logic
    AnimatedScalarSpec warp_strength{0.0};
    AnimatedScalarSpec warp_frequency{5.0};
    AnimatedScalarSpec warp_speed{2.0};
    
    // Post-Processing / Stylization
    AnimatedScalarSpec grain_amount{0.0};
    AnimatedScalarSpec grain_scale{2.0};
    AnimatedScalarSpec scanline_intensity{0.0};
    AnimatedScalarSpec scanline_frequency{100.0};
    AnimatedScalarSpec contrast{1.0};
    AnimatedScalarSpec gamma{1.0};
    AnimatedScalarSpec saturation{1.0};
    AnimatedScalarSpec softness{0.0}; // Blur/softness factor for the pattern

    // Aurora / Advanced Noise
    AnimatedScalarSpec octave_decay{0.5};
    AnimatedScalarSpec band_height{0.5};
    AnimatedScalarSpec band_spread{1.0};

    [[nodiscard]] bool empty() const noexcept {
        return kind.empty();
    }
};

struct ParticleSpec {
    // Emitter settings
    std::string emitter_shape{"point"}; // "point", "circle", "rect"
    AnimatedScalarSpec emission_rate{100.0}; // particles per second
    AnimatedScalarSpec lifetime{2.0}; // seconds
    AnimatedScalarSpec start_speed{100.0}; // initial velocity magnitude
    AnimatedScalarSpec direction{0.0}; // angle in degrees
    AnimatedScalarSpec spread{360.0}; // emission spread angle
    
    // Physics
    AnimatedScalarSpec gravity{980.0}; // pixels/s^2
    AnimatedScalarSpec drag{0.0}; // velocity damping
    AnimatedScalarSpec turbulence{0.0}; // random force
    
    // Appearance
    AnimatedScalarSpec start_size{5.0};
    AnimatedScalarSpec end_size{0.0};
    AnimatedColorSpec start_color{ColorSpec{255, 255, 255, 255}};
    AnimatedColorSpec end_color{ColorSpec{255, 255, 255, 0}};
    
    // Blend mode for particles
    std::string blend_mode{"normal"};
    
    [[nodiscard]] bool empty() const noexcept {
        return emitter_shape.empty();
    }
};

struct LayerTransitionSpec {
    std::string type{"none"}; // DEPRECATED: Use transition_id instead. "fade", "slide", "zoom", "flip", "blur"
    std::string direction{"none"}; // "up", "down", "left", "right", "random"
    double duration{0.4};
    animation::EasingPreset easing{animation::EasingPreset::EaseOut};
    animation::SpringEasing spring{}; // Custom spring parameters
    double delay{0.0};
    std::string transition_id; // Unified transition registry ID (e.g., "fade", "slide", "circle_iris", "fade_to_black")
};

struct InstanceSpec {
    std::string source;
    std::map<std::string, nlohmann::json> overrides;
};

struct LayerSpec {
    struct MarkerSpec {
        double time{0.0};
        std::string label;
        std::string color;
    };

    std::string id;
    std::string name;
    std::string type; // "solid", "shape", "image", "text", "precomp", etc.
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
    std::optional<double> duration;

    int width{1920};
    int height{1080};

    Transform2D transform;
    Transform3D transform3d;
    
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
    AnimatedColorSpec fill_color;
    AnimatedColorSpec stroke_color;
    double stroke_width{0.0};
    AnimatedScalarSpec stroke_width_property;
    double extrusion_depth{0.0};
    double bevel_size{0.0};
    double hole_bevel_ratio{0.0};
    
    // Variable font axes animation (e.g., wght, wdth, opsz)
    std::map<std::string, AnimatedScalarSpec> font_axes;
    
    // Subtitle & Word Timestamps
    std::string subtitle_path;
    std::string word_timestamp_path;
    double subtitle_outline_width{0.0};
    AnimatedColorSpec subtitle_outline_color;

    // Vector Graphics / Shapes
    std::string line_cap{"butt"};
    std::string line_join{"miter"};
    double miter_limit{4.0};
    std::optional<ShapePathSpec> shape_path;
    std::optional<ShapeSpec> shape_spec;
    std::vector<renderer2d::MaskPath> mask_paths;

    // Gradient fill (animated)
    std::optional<AnimatedGradientSpec> gradient_fill;
    
    // Procedural layer
    std::optional<ProceduralSpec> procedural;
    
    // Particle system layer
    std::optional<ParticleSpec> particle_spec;
    
    // Effects & Animators
    std::vector<EffectSpec> effects;
    std::vector<AnimatedEffectSpec> animated_effects;
    std::vector<TextAnimatorSpec> text_animators;
    std::vector<TextHighlightSpec> text_highlights;

    // Repeater
    std::string        repeater_type{"linear"}; // "linear", "grid", "radial"
    AnimatedScalarSpec repeater_count;
    AnimatedScalarSpec repeater_grid_cols;
    AnimatedScalarSpec repeater_radial_radius;
    AnimatedScalarSpec repeater_radial_start_angle;
    AnimatedScalarSpec repeater_radial_end_angle;
    AnimatedScalarSpec repeater_stagger_delay;
    AnimatedScalarSpec repeater_offset_position_x;
    AnimatedScalarSpec repeater_offset_position_y;
    AnimatedScalarSpec repeater_offset_rotation;
    AnimatedScalarSpec repeater_offset_scale_x;
    AnimatedScalarSpec repeater_offset_scale_y;
    AnimatedScalarSpec repeater_start_opacity;
    AnimatedScalarSpec repeater_end_opacity;

    // Camera
    std::string camera_type{"one_node"};
    AnimatedScalarSpec camera_zoom;
    AnimatedVector3Spec camera_poi;
    uint64_t camera_shake_seed{0};
    AnimatedScalarSpec camera_shake_amplitude_pos;
    AnimatedScalarSpec camera_shake_amplitude_rot;
    AnimatedScalarSpec camera_shake_frequency;
    AnimatedScalarSpec camera_shake_roughness;

    // Light
    std::string light_type{"point"};
    AnimatedScalarSpec light_intensity;
    AnimatedColorSpec light_color;
    std::string falloff_type{"inverse_square"};
    AnimatedScalarSpec attenuation_near;
    AnimatedScalarSpec attenuation_far;
    AnimatedScalarSpec cone_angle;
    AnimatedScalarSpec cone_feather;
    bool casts_shadows{true};
    AnimatedScalarSpec shadow_darkness;
    AnimatedScalarSpec shadow_radius;

    // Temporal & Tracking
    std::vector<spec::TrackBinding> track_bindings;
    spec::TimeRemapCurve time_remap;
    spec::FrameBlendMode frame_blend{spec::FrameBlendMode::None};

    // Parallax & 2D Camera
    bool has_parallax{false};
    std::optional<std::string> camera2d_id;
    float parallax_factor{1.0f};

    // Animation transitions
    LayerTransitionSpec transition_in;
    LayerTransitionSpec transition_out;
    std::string during_preset;

    // Playback behavior
    bool loop{false};
    bool hold_last_frame{false};

    // Instance data (if type == "instance")
    std::optional<InstanceSpec> instance;

    // Markers
    std::vector<MarkerSpec> markers;

    // Cache
    std::uint64_t spec_hash{0};
};

// JSON serialization declarations (implementations in layer_spec_serialize.cpp)
void to_json(nlohmann::json& j, const ProceduralSpec& p);
void from_json(const nlohmann::json& j, ProceduralSpec& p);
void to_json(nlohmann::json& j, const ParticleSpec& p);
void from_json(const nlohmann::json& j, ParticleSpec& p);

} // namespace tachyon
