#pragma once

#include "tachyon/core/spec/schema/objects/procedural_spec.h"
#include "tachyon/core/spec/schema/effects/effect_spec.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/core/spec/schema/objects/text_box_spec.h"
#include "tachyon/core/spec/schema/objects/path_spec.h"

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

struct LayerTransitionSpec {
    std::string transition_id{"none"};
    Direction direction{Direction::None};
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

struct LayerTextSpec {
    std::string content;
    std::string font_id;
    AnimatedScalarSpec font_size;
    TextBoxSpec box;
    std::map<std::string, AnimatedScalarSpec> font_axes;
    AnimatedColorSpec fill_color;
    AnimatedColorSpec stroke_color;
    double stroke_width{0.0};
    AnimatedScalarSpec stroke_width_property;
};

struct LayerSubtitleSpec {
    std::string path;
    AnimatedColorSpec outline_color;
    double outline_width{2.0};
    std::string word_timestamp_path;
};

struct LayerRepeaterSpec {
    std::string type{"linear"};
    AnimatedScalarSpec count;
    AnimatedScalarSpec stagger_delay;
    AnimatedScalarSpec offset_position_x;
    AnimatedScalarSpec offset_position_y;
    AnimatedScalarSpec offset_rotation;
    AnimatedScalarSpec offset_scale_x;
    AnimatedScalarSpec offset_scale_y;
    AnimatedScalarSpec start_opacity;
    AnimatedScalarSpec end_opacity;
    AnimatedScalarSpec grid_cols;
    AnimatedScalarSpec radial_radius;
    AnimatedScalarSpec radial_start_angle;
    AnimatedScalarSpec radial_end_angle;
};

struct LayerVectorSpec {
    std::optional<ShapePathSpec> shape_path;
    std::optional<ShapeSpec> shape_spec;
    spec::LineCap line_cap{spec::LineCap::Butt};
    spec::LineJoin line_join{spec::LineJoin::Miter};
    double miter_limit{4.0};
};

struct LayerMaskSpec {
    std::vector<spec::MaskPath> paths;
    AnimatedScalarSpec feather;
};

struct LayerTemporalSpec {
    std::vector<spec::TrackBinding> track_bindings;
    spec::TimeRemapCurve time_remap;
    spec::FrameBlendMode frame_blend{spec::FrameBlendMode::Linear};
    AnimatedScalarSpec time_remap_property;
};

struct LayerIdentity {
    std::string id;
    std::string name;
    LayerType type{LayerType::NullLayer};
    bool enabled{true};
    bool visible{true};
    bool is_adjustment_layer{false};
    bool motion_blur{false};
};

struct LayerSource {
    std::string asset_id; // For image/video layers
    std::string preset_id; // For procedural/preset layers
    std::optional<std::string> precomp_id;
    std::optional<ProceduralSpec> procedural;
};

struct LayerTransform {
    Transform2D transform;
    int width{1920};
    int height{1080};
    double opacity{1.0};
    AnimatedScalarSpec opacity_property;
    
    // Camera integration
    bool has_parallax{true};
    float parallax_factor{1.0f};
    std::optional<std::string> camera2d_id;
};

struct LayerPlayback {
    LayerTiming timing;
    LayerTemporalSpec temporal;
    bool loop{false};
    bool hold_last_frame{false};
    
    struct MarkerSpec {
        double time{0.0};
        std::string label;
        std::string color{"#ffffff"};
    };
    std::vector<MarkerSpec> markers;
};

struct LayerSpec {
    LayerIdentity identity;
    LayerSource source;
    LayerTransform transform;
    LayerPlayback playback;

    BlendMode blend_mode{BlendMode::Normal};

    // Component Specs
    LayerTextSpec text;
    LayerSubtitleSpec subtitles;
    LayerRepeaterSpec repeater;
    LayerVectorSpec vector;
    LayerMaskSpec masks;

    // Effects & Animators
    std::vector<EffectSpec> effects;
    std::vector<TextAnimatorSpec> text_animators;
    std::vector<TextHighlightSpec> text_highlights;

    // Presets & Transitions
    std::string animation_in_preset;
    std::string animation_during_preset;
    std::string animation_out_preset;

    LayerTransitionSpec transition_in;
    LayerTransitionSpec transition_out;

    std::optional<std::string> parent;
    std::optional<std::string> track_matte_layer_id;
    TrackMatteType track_matte_type{TrackMatteType::None};
};

} // namespace tachyon
