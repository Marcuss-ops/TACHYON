#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/runtime/diagnostics.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace tachyon {

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
};

struct Vector2KeyframeSpec {
    double time{0.0};
    math::Vector2 value{math::Vector2::zero()};
};

struct AnimatedScalarSpec {
    std::optional<double> value;
    std::vector<ScalarKeyframeSpec> keyframes;

    [[nodiscard]] bool empty() const noexcept {
        return !value.has_value() && keyframes.empty();
    }
};

struct AnimatedVector2Spec {
    std::optional<math::Vector2> value;
    std::vector<Vector2KeyframeSpec> keyframes;

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

struct LayerSpec {
    std::string id;
    std::string type;
    std::string name;
    bool enabled{true};
    double start_time{0.0};
    double in_point{0.0};
    double out_point{0.0};
    double opacity{1.0};
    std::optional<std::string> parent;
    Transform2D transform;
    AnimatedScalarSpec opacity_property;
    AnimatedScalarSpec time_remap_property;
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
