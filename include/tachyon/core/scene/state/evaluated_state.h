#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/objects/procedural_spec.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"

#include "tachyon/core/math/matrix3x3.h"
#include "tachyon/core/math/transform2.h"
#include "tachyon/core/shapes/shape_path.h"
#include "tachyon/core/spec/schema/common/gradient_spec.h"
#include "tachyon/core/spec/schema/objects/mask_spec.h"
#include "tachyon/core/spec/schema/objects/path_spec.h"
#include "tachyon/core/spec/schema/objects/text_box_spec.h"
#include "tachyon/text/content/word_timestamps.h"
#include <vector>
#include <string>
#include <memory>
#include <optional>


namespace tachyon::scene {

using LayerType = ::tachyon::LayerType;
using TrackMatteType = ::tachyon::TrackMatteType;

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
    
    math::Matrix3x3 world_matrix{math::Matrix3x3()};
    math::Transform2 local_transform{math::Transform2::identity()};
    bool motion_blur{false};
    
    bool visible{true};
    bool enabled{true};
    bool active{true};
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
    std::vector<EffectSpec> animated_effects;
    std::vector<EffectSpec> effects;
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
    TextBoxSpec text_box;
    math::Vector2 resolved_box_size{0.0f, 0.0f};
    std::string text_content;
    std::string font_id;
    float font_size{0.0f};
    
    std::optional<std::string> texture_asset_id;

    std::optional<std::size_t> track_matte_layer_index;
    TrackMatteType track_matte_type{TrackMatteType::None};
    std::optional<std::string> precomp_id;
    bool collapse_transformations{false}; // Merge precomp layers into parent comp when true
    
    std::shared_ptr<EvaluatedCompositionState> nested_composition;
    std::vector<math::Vector2> corner_pin;
    

    // Previous state for motion blur calculation
    math::Matrix3x3 previous_world_matrix{math::Matrix3x3()};

    // Masks
    std::vector<spec::MaskPath> masks;
    // Trim Paths support
    float trim_start{0.0f};
    float trim_end{1.0f};
    float trim_offset{0.0f};

    bool text_on_path_enabled{false};

    // Text Animation
    std::vector<TextAnimatorSpec> text_animators;
    std::vector<TextHighlightSpec> text_highlights;

    // Transitions
    LayerTransitionSpec transition_in;
    LayerTransitionSpec transition_out;
    double in_time{0.0};
    double out_time{10.0};

    // Procedural generation
    std::optional<ProceduralSpec> procedural;
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
    
    ColorSpec background_color{0, 0, 0, 0};


};

} // namespace tachyon::scene
