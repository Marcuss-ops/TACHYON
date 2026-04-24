/**
 * @file scene_spec.h
 * @brief Authoring specifications for the Tachyon rendering engine.
 */

#pragma once

#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/core/spec/schema/assets/asset_spec.h"
#include "tachyon/core/spec/schema/properties/property_spec.h"
#include "tachyon/core/spec/schema/transform/transform_spec.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"
#include "tachyon/text/fonts/font_manifest.h"

#include <filesystem>
#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace tachyon {

struct ProjectSpec {
    std::string id;
    std::string name;
    std::string authoring_tool;
    std::optional<std::int64_t> root_seed;
};

struct SceneSpec {
    std::string version{"1.0"};
    std::string spec_version{"1.0.0"};
    ProjectSpec project;
    std::vector<CompositionSpec> compositions;
    std::vector<AssetSpec> assets;
    std::vector<DataSourceSpec> data_sources;
    std::optional<text::FontManifest> font_manifest;
    std::optional<std::string> font_manifest_path;
};

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
        std::optional<std::string> background = std::nullopt);

    SceneSpec Build() &&;

private:
    struct SceneSpecBuilderImpl;
    std::unique_ptr<SceneSpecBuilderImpl> impl_;
};

} // namespace tachyon
