#pragma once

#include "tachyon/diagnostics.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace tachyon {

struct AssetSpec {
    std::string id;
    std::string type;
    std::string source;
};

struct ProjectSpec {
    std::string id;
    std::string name;
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

struct Transform2D {
    std::optional<double> position_x;
    std::optional<double> position_y;
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
};

struct CompositionSpec {
    std::string id;
    std::string name;
    std::int64_t width{0};
    std::int64_t height{0};
    double duration{0.0};
    FrameRate frame_rate;
    std::optional<std::string> background;
    std::vector<LayerSpec> layers;
};

struct RenderDefaults {
    std::optional<std::string> output;
};

struct SceneSpec {
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
