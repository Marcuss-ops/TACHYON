#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/animation/easing.h"
#include "tachyon/runtime/diagnostics.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

namespace tachyon {

enum class TrackMatteType {
    None,
    Alpha,
    AlphaInverted,
    Luma,
    LumaInverted
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
};

struct Vector2KeyframeSpec {
    double time{0.0};
    math::Vector2 value{math::Vector2::zero()};
    animation::EasingPreset easing{animation::EasingPreset::None};
    animation::CubicBezierEasing bezier{animation::CubicBezierEasing::linear()};
};

struct ColorSpec {
    std::uint8_t r{255}, g{255}, b{255}, a{255};
};

struct EffectSpec {
    std::string type;
    std::string id;
    bool enabled{true};
    std::unordered_map<std::string, double> scalars;
    std::unordered_map<std::string, ColorSpec> colors;
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

    [[nodiscard]] bool empty() const noexcept {
        return !value.has_value() && keyframes.empty() && !audio_band.has_value();
    }
};

struct AnimatedVector2Spec {
    std::optional<math::Vector2> value;
    std::vector<Vector2KeyframeSpec> keyframes;

    [[nodiscard]] bool empty() const noexcept {
        return !value.has_value() && keyframes.empty();
    }
};

struct AnimatedVector3Spec {
    std::optional<math::Vector3> value;
    struct Keyframe {
        double time{0.0};
        math::Vector3 value{math::Vector3::zero()};
        animation::EasingPreset easing{animation::EasingPreset::None};
        animation::CubicBezierEasing bezier{animation::CubicBezierEasing::linear()};
    };
    std::vector<Keyframe> keyframes;

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
    AnimatedVector3Spec rotation_property; // Euler angles in degrees
    AnimatedVector3Spec scale_property;
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
    ColorSpec fill_color{255, 255, 255, 255};
    ColorSpec stroke_color{0, 0, 0, 255};
    std::optional<std::string> parent;
    bool is_3d{false};
    bool is_adjustment_layer{false};
    Transform2D transform;
    Transform3D transform3d;
    std::optional<ShapePathSpec> shape_path;
    std::vector<EffectSpec> effects;
    AnimatedScalarSpec opacity_property;
    AnimatedScalarSpec time_remap_property;
    TrackMatteType track_matte_type{TrackMatteType::None};
    std::optional<std::string> track_matte_layer_id;
    std::optional<std::string> precomp_id;
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
    std::vector<LayerSpec> layers;
};

struct RenderDefaults {
    std::optional<std::string> output;
};

struct SceneSpec {
    std::string version;
    std::string spec_version;
    ProjectSpec project;
    std::vector<AssetSpec> assets;
    std::vector<CompositionSpec> compositions;
    RenderDefaults render_defaults;
};

ParseResult<SceneSpec> parse_scene_spec_json(const std::string& text);
ParseResult<SceneSpec> parse_scene_spec_file(const std::filesystem::path& path);
ValidationResult validate_scene_spec(const SceneSpec& scene);

} // namespace tachyon
