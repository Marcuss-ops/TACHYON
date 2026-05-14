#pragma once

#include "tachyon/core/spec/schema/objects/procedural_spec.h"
#include "tachyon/core/spec/schema/effects/effect_spec.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/core/spec/schema/objects/text_box_spec.h"

#include "tachyon/core/spec/schema/common/common_spec.h"

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
    std::string transition_id{"none"};
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
    std::string preset_id; // For procedural/preset layers
    LayerType type{LayerType::NullLayer}; // Canonical Source of Truth
    std::string blend_mode{"normal"};
    
    bool enabled{true};
    bool visible{true};
    bool is_adjustment_layer{false};
    bool motion_blur{false};

    LayerTiming timing;
    double opacity{1.0};

    int width{1920};
    int height{1080};

    Transform2D transform;
    
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
    TextBoxSpec text_box;
    std::map<std::string, AnimatedScalarSpec> font_axes;
    AnimatedColorSpec fill_color;
    AnimatedColorSpec stroke_color;
    double stroke_width{0.0};
    AnimatedScalarSpec stroke_width_property;

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
    

    // 2D Camera integration
    bool has_parallax{true};
    float parallax_factor{1.0f};
    std::optional<std::string> camera2d_id;

    // Effects & Animators (Skeletons)
    std::vector<EffectSpec> effects;
    std::vector<TextAnimatorSpec> text_animators;
    std::vector<TextHighlightSpec> text_highlights;

    // Mask paths (roto / vector masks)
    std::vector<spec::MaskPath> mask_paths;

    // Temporal & Tracking (Unified)
    std::vector<spec::TrackBinding> track_bindings;
    spec::TimeRemapCurve time_remap;
    spec::FrameBlendMode frame_blend{spec::FrameBlendMode::Linear};

    // Animation preset durations (inject transform/opacity keyframes via PresetCompiler)
    // Animation presets (inject transform/opacity keyframes via PresetCompiler)
    // Modern animation preset names
    std::string animation_in_preset;
    std::string animation_during_preset;
    std::string animation_out_preset;

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
};

} // namespace tachyon
