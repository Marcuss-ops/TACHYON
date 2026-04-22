#pragma once

#include "tachyon/core/spec/layer_spec.h"
#include "tachyon/core/spec/asset_spec.h"
#include "tachyon/runtime/core/diagnostics.h"

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <filesystem>

namespace tachyon {

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

struct SceneSpec {
    std::string version;
    std::string spec_version;
    ProjectSpec project;
    std::vector<AssetSpec> assets;
    std::vector<DataSourceSpec> data_sources;
    ColorManagementSpec color_management;
    std::vector<CompositionSpec> compositions;
    RenderDefaults render_defaults;
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
