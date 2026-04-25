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
    std::string kind{"noise"}; // "noise" | "waves" | "stripes" | "gradient_sweep"
    
    // All parameters use existing Animated* types - reusable with expression VM
    AnimatedScalarSpec frequency{1.0};
    AnimatedScalarSpec speed{1.0};
    AnimatedScalarSpec amplitude{1.0};
    AnimatedScalarSpec scale{1.0};
    
    AnimatedColorSpec color_a{ColorSpec{255, 0, 0, 255}};
    AnimatedColorSpec color_b{ColorSpec{0, 0, 255, 255}};
    
    uint64_t seed{0};
    
    // For waves/stripes
    AnimatedScalarSpec angle{0.0};
    AnimatedScalarSpec spacing{50.0};
    
    [[nodiscard]] bool empty() const noexcept {
        return kind.empty();
    }
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

    // Effects & Animators
    std::vector<EffectSpec> effects;
    std::vector<AnimatedEffectSpec> animated_effects;
    std::vector<TextAnimatorSpec> text_animators;
    std::vector<TextHighlightSpec> text_highlights;

    // Repeater
    AnimatedScalarSpec repeater_count;
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

    // Animation presets
    std::string in_preset;
    std::string during_preset;
    std::string out_preset;
    double in_duration{0.4};
    double out_duration{0.4};

    // Playback behavior
    bool loop{false};
    bool hold_last_frame{false};

    // Markers
    std::vector<MarkerSpec> markers;
};

// JSON serialization for ProceduralSpec
inline void to_json(nlohmann::json& j, const ProceduralSpec& p) {
    j["kind"] = p.kind;
    if (!p.frequency.empty()) j["frequency"] = p.frequency;
    if (!p.speed.empty()) j["speed"] = p.speed;
    if (!p.amplitude.empty()) j["amplitude"] = p.amplitude;
    if (!p.scale.empty()) j["scale"] = p.scale;
    if (!p.color_a.empty()) j["color_a"] = p.color_a;
    if (!p.color_b.empty()) j["color_b"] = p.color_b;
    if (p.seed != 0) j["seed"] = p.seed;
    if (!p.angle.empty()) j["angle"] = p.angle;
    if (!p.spacing.empty()) j["spacing"] = p.spacing;
}

inline void from_json(const nlohmann::json& j, ProceduralSpec& p) {
    if (j.contains("kind") && j.at("kind").is_string()) p.kind = j.at("kind").get<std::string>();
    if (j.contains("frequency")) p.frequency = j.at("frequency").get<AnimatedScalarSpec>();
    if (j.contains("speed")) p.speed = j.at("speed").get<AnimatedScalarSpec>();
    if (j.contains("amplitude")) p.amplitude = j.at("amplitude").get<AnimatedScalarSpec>();
    if (j.contains("scale")) p.scale = j.at("scale").get<AnimatedScalarSpec>();
    if (j.contains("color_a")) p.color_a = j.at("color_a").get<AnimatedColorSpec>();
    if (j.contains("color_b")) p.color_b = j.at("color_b").get<AnimatedColorSpec>();
    if (j.contains("seed") && j.at("seed").is_number()) p.seed = j.at("seed").get<uint64_t>();
    if (j.contains("angle")) p.angle = j.at("angle").get<AnimatedScalarSpec>();
    if (j.contains("spacing")) p.spacing = j.at("spacing").get<AnimatedScalarSpec>();
}

} // namespace tachyon
