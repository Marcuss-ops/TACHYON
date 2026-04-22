/**
 * @file scene_spec.h
 * @brief Authoring specifications for the Tachyon rendering engine.
 * 
 * This file defines the "Authoring Boundary" (SceneSpec). These structures are 
 * designed for ease of use by authoring tools (JSON/YAML parsers) and are not 
 * optimized for runtime execution.
 * 
 * Hierarchy:
 * - SceneSpec: The root container for a project.
 *   - CompositionSpec: A renderable sequence (timeline).
 *     - LayerSpec: An individual element (Transform, Opacity, Effects).
 *       - AnimatedScalarSpec/AnimatedVectorSpec: Property tracks with keyframes or expressions.
 */

#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/animation/easing.h"
#include "tachyon/runtime/core/diagnostics.h"
#include "tachyon/renderer2d/raster/path_rasterizer.h"
#include "tachyon/renderer2d/spec/gradient_spec.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace tachyon {

enum class TrackMatteType {
    None,
    Alpha,
    AlphaInverted,
    Luma,
    LumaInverted
};

enum class LayerType {
    Solid,
    Shape,
    Image,
    Video,
    Text,
    Camera,
    Precomp,
    Light,
    Mask,
    NullLayer,
    Unknown
};

enum class AudioBandType {
    Bass,
    Mid,
    High,
    Presence,
    Rms
};

struct AssetSpec {
    std::string id;
    std::string type;
    std::string path;
    std::string source;
    std::optional<std::string> alpha_mode;
};

struct DataSourceSpec {
    std::string id;
    std::string path;
    std::string type; // "csv", "json"
};

struct ColorManagementSpec {
    std::string workflow{"gamma"}; // "linear", "gamma"
    std::string working_space{"srgb"}; // "srgb", "acescg", "rec709"
    std::string output_space{"srgb"};
};

struct ProjectSpec {
    std::string id;
    std::string name;
    std::string authoring_tool;
    std::optional<std::int64_t> root_seed;
};

struct FrameRate {
    std::int64_t numerator{0};
    std::int64_t denominator{1};

    [[nodiscard]] double value() const noexcept {
        if (denominator == 0) {
            return 0.0;
        }
        return static_cast<double>(numerator) / static_cast<double>(denominator);
    }
};

struct ScalarKeyframeSpec {
    double time{0.0};
    double value{0.0};
    animation::EasingPreset easing{animation::EasingPreset::None};
    animation::CubicBezierEasing bezier{animation::CubicBezierEasing::linear()};
    
    double speed_in{0.0};
    double influence_in{33.333333333};
    double speed_out{0.0};
    double influence_out{33.333333333};
};

struct Vector2KeyframeSpec {
    double time{0.0};
    math::Vector2 value{math::Vector2::zero()};
    math::Vector2 tangent_in{math::Vector2::zero()};
    math::Vector2 tangent_out{math::Vector2::zero()};

    animation::EasingPreset easing{animation::EasingPreset::None};
    animation::CubicBezierEasing bezier{animation::CubicBezierEasing::linear()};

    double speed_in{0.0};
    double influence_in{33.333333333};
    double speed_out{0.0};
    double influence_out{33.333333333};
};


struct ColorKeyframeSpec {
    double time{0.0};
    ColorSpec value{255, 255, 255, 255};
    animation::EasingPreset easing{animation::EasingPreset::None};
    animation::CubicBezierEasing bezier{animation::CubicBezierEasing::linear()};

    double speed_in{0.0};
    double influence_in{33.333333333};
    double speed_out{0.0};
    double influence_out{33.333333333};
};

struct EffectSpec {
    std::string type;
    std::string id;
    bool enabled{true};
    std::unordered_map<std::string, double> scalars;
    std::unordered_map<std::string, ColorSpec> colors;
    std::unordered_map<std::string, std::string> strings;
};

struct ShapePathPointSpec {
    math::Vector2 position{math::Vector2::zero()};
    math::Vector2 tangent_in{math::Vector2::zero()};
    math::Vector2 tangent_out{math::Vector2::zero()};
};

struct ShapePathSpec {
    std::vector<ShapePathPointSpec> points;
    bool closed{true};
};

struct AnimatedScalarSpec {
    std::optional<double> value;
    std::vector<ScalarKeyframeSpec> keyframes;
    std::optional<AudioBandType> audio_band;
    double audio_min{0.0};
    double audio_max{1.0};
    std::optional<std::string> expression;

    [[nodiscard]] bool empty() const noexcept {
        return !value.has_value() && keyframes.empty() && !audio_band.has_value() && !expression.has_value();
    }
};

struct AnimatedVector2Spec {
    std::optional<math::Vector2> value;
    std::vector<Vector2KeyframeSpec> keyframes;
    std::optional<std::string> expression;

    [[nodiscard]] bool empty() const noexcept {
        return !value.has_value() && keyframes.empty() && !expression.has_value();
    }
};

struct AnimatedVector3Spec {
    std::optional<math::Vector3> value;
    struct Keyframe {
        double time{0.0};
        math::Vector3 value{math::Vector3::zero()};
        math::Vector3 tangent_in{math::Vector3::zero()};
        math::Vector3 tangent_out{math::Vector3::zero()};

        animation::EasingPreset easing{animation::EasingPreset::None};
        animation::CubicBezierEasing bezier{animation::CubicBezierEasing::linear()};

        double speed_in{0.0};
        double influence_in{33.333333333};
        double speed_out{0.0};
        double influence_out{33.333333333};
    };
    std::vector<Keyframe> keyframes;
    std::optional<std::string> expression;

    [[nodiscard]] bool empty() const noexcept {
        return !value.has_value() && keyframes.empty() && !expression.has_value();
    }
};

struct AnimatedColorSpec {
    std::optional<ColorSpec> value;
    std::vector<ColorKeyframeSpec> keyframes;

    [[nodiscard]] bool empty() const noexcept {
        return !value.has_value() && keyframes.empty();
    }
};

struct Transform2D {
    std::optional<double> position_x;
    std::optional<double> position_y;
    std::optional<double> rotation;
    std::optional<double> scale_x;
    std::optional<double> scale_y;
    AnimatedVector2Spec position_property;
    AnimatedScalarSpec rotation_property;
    AnimatedVector2Spec scale_property;
};

struct Transform3D {
    AnimatedVector3Spec position_property;
    AnimatedVector3Spec orientation_property; // AE-style orientation (degrees)
    AnimatedVector3Spec rotation_property;    // Euler angles (degrees)
    AnimatedVector3Spec scale_property;
    AnimatedVector3Spec anchor_point_property;
};

// ---------------------------------------------------------------------------
// Text Animator structs (used by LayerSpec for type == "text" layers)
// ---------------------------------------------------------------------------

/// Defines which characters are affected by an animator and by how much.
/// For "range" type, character i of N total gets:
///   coverage = clamp((i/N - start) / (end - start), 0, 1)
struct TextAnimatorSelectorSpec {
    std::string type{"range"}; // "range" | "all"
    double start{0.0};
    double end{1.0};
};

/// Animatable per-character properties driven by a TextAnimatorSpec.
/// Each property can hold a static value OR keyframes — not both.
/// The evaluated value is then scaled by the selector coverage.
struct TextAnimatorPropertySpec {
    // Opacity in [0, 1] (1 = fully opaque)
    std::optional<double>            opacity_value;
    std::vector<ScalarKeyframeSpec>  opacity_keyframes;

    // Position offset in pixels (local glyph space)
    std::optional<math::Vector2>          position_offset_value;
    std::vector<Vector2KeyframeSpec>      position_offset_keyframes;

    // Uniform scale around glyph centre (1 = no change)
    std::optional<double>            scale_value;
    std::vector<ScalarKeyframeSpec>  scale_keyframes;

    // Rotation in degrees around glyph centre
    std::optional<double>            rotation_value;
    std::vector<ScalarKeyframeSpec>  rotation_keyframes;
};

struct TextAnimatorSpec {
    TextAnimatorSelectorSpec  selector;
    TextAnimatorPropertySpec  properties;
};

struct TextHighlightSpec {
    std::size_t start_glyph{0};
    std::size_t end_glyph{0}; // exclusive
    ColorSpec color{255, 236, 59, 96};
    std::int32_t padding_x{3};
    std::int32_t padding_y{2};
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
    
    // Trim Path properties
    AnimatedScalarSpec trim_start;
    AnimatedScalarSpec trim_end;
    AnimatedScalarSpec trim_offset;
    
    // Repeater properties
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
    
    // Text animator array (for type == "text" layers only)
    std::vector<TextAnimatorSpec> text_animators;
    std::vector<TextHighlightSpec> text_highlights;

    // Subtitle burn-in (for type == "text" layers with subtitle_path set)
    std::optional<std::string> subtitle_path;
    std::optional<ColorSpec>   subtitle_outline_color;
    float                      subtitle_outline_width{0.0f};

    // Material options (compact PBR)
    AnimatedScalarSpec ambient_coeff;
    AnimatedScalarSpec diffuse_coeff;
    AnimatedScalarSpec specular_coeff;
    AnimatedScalarSpec shininess;
    AnimatedScalarSpec metallic;
    AnimatedScalarSpec roughness;
    AnimatedScalarSpec emission;
    AnimatedScalarSpec ior;
    bool metal{false};

    // Advanced 3D / Extrusion
    AnimatedScalarSpec extrusion_depth;
    AnimatedScalarSpec bevel_size;

    // Camera specific
    bool               is_two_node{true};
    AnimatedVector3Spec point_of_interest;
    bool               dof_enabled{false};
    AnimatedScalarSpec focus_distance;
    AnimatedScalarSpec aperture;
    AnimatedScalarSpec blur_level;
    AnimatedScalarSpec aperture_blades;
    AnimatedScalarSpec aperture_rotation;

    // Light specific
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
};

struct CompositionSpec {
    std::string id;
    std::string name;
    std::int64_t width{0};
    std::int64_t height{0};
    std::optional<std::int64_t> fps;
    double duration{0.0};
    FrameRate frame_rate;
    std::optional<std::string> background;
    std::optional<std::string> environment_path;
    std::vector<LayerSpec> layers;
};

struct RenderDefaults {
    std::optional<std::string> output;
};

/**
 * @brief Top-level container for a project or multi-composition scene.
 */
struct SceneSpec {
    std::string version;      ///< Tachyon schema version.
    std::string spec_version; ///< Specific project spec version.
    ProjectSpec project;      ///< Global project metadata.
    std::vector<AssetSpec> assets;             ///< External assets (images, video, audio).
    std::vector<DataSourceSpec> data_sources;   ///< External data sources (csv, json).
    ColorManagementSpec color_management;       ///< Color management settings.
    std::vector<CompositionSpec> compositions; ///< All compositions in the project.
    RenderDefaults render_defaults;           ///< Default settings for output.
};

ParseResult<SceneSpec> parse_scene_spec_json(const std::string& text);
ParseResult<SceneSpec> parse_scene_spec_file(const std::filesystem::path& path);
ValidationResult validate_scene_spec(const SceneSpec& scene);

class SceneSpecBuilder {
public:
    SceneSpecBuilder();
    ~SceneSpecBuilder();
    
    SceneSpecBuilder& SetProjectId(std::string id);
    SceneSpecBuilder& SetProjectName(std::string name);
    SceneSpecBuilder& SetProjectAuthoringTool(std::string tool);
    SceneSpecBuilder& SetProjectRootSeed(std::optional<std::int64_t> seed);
    
    SceneSpecBuilder& AddComposition(
        std::string id, std::string name, std::int64_t width,
        std::int64_t height, double duration, std::optional<std::int64_t> fps,
        std::optional<std::string> background = {});
        
    SceneSpec Build() &&;
    
private:
    struct SceneSpecBuilderImpl;
    std::unique_ptr<SceneSpecBuilderImpl> impl_;
};

} // namespace tachyon
