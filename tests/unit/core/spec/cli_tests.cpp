#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/validation/layer_spec_normalizer.h"
#include "tachyon/core/spec/validation/scene_validator.h"
#include "tachyon/scene/builder.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

struct StreamCapture {
    std::ostream& stream;
    std::streambuf* previous;
    std::ostringstream buffer;

    explicit StreamCapture(std::ostream& target)
        : stream(target), previous(target.rdbuf(buffer.rdbuf())) {}

    ~StreamCapture() {
        stream.rdbuf(previous);
    }

    std::string str() const {
        return buffer.str();
    }
};

} // namespace

bool run_cli_tests() {
    g_failures = 0;

    {
        tachyon::SceneSpec scene;
        scene.project.id = "proj_cli_test";
        scene.project.name = "CLI Test";
        scene.compositions.push_back(
            tachyon::scene::Composition("main")
                .size(1920, 1080)
                .fps(30)
                .duration(10.0)
                .build());
        check_true(!scene.compositions.empty(), "CLI test scene should have compositions");
    }

    {
        std::vector<std::string> args = {"tachyon", "validate", "--cpp", "/nonexistent/path/test.cpp"};
        std::vector<char*> argv;
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }

        StreamCapture capture_out(std::cout);
        const int exit_code = tachyon::run_cli(static_cast<int>(argv.size()), argv.data());
        // exit_code should be 1 (failed to load) but NOT related to argument parsing error
        check_true(exit_code != 0, "CLI should fail on non-existent C++ file");
    }

    {
        std::vector<std::string> args = {"tachyon", "render", "--cpp", "scene.cpp", "--output-preset", "youtube_shorts"};
        std::vector<char*> argv;
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }

        const auto parsed = tachyon::parse_cli_options(static_cast<int>(argv.size()), argv.data());
        check_true(parsed.value.has_value(), "CLI parser should accept --output-preset");
        if (parsed.value.has_value()) {
            check_true(parsed.value->output_preset_id.has_value(), "CLI parser should store output preset id");
            check_true(parsed.value->output_preset_id.value() == "youtube_shorts", "CLI parser should preserve output preset id");
        }
    }

    {
        std::vector<std::string> args = {"tachyon", "output-presets", "list"};
        std::vector<char*> argv;
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }

        std::ostringstream out;
        std::ostringstream err;
        auto* old_out = std::cout.rdbuf(out.rdbuf());
        auto* old_err = std::cerr.rdbuf(err.rdbuf());
        const int exit_code = tachyon::run_cli(static_cast<int>(argv.size()), argv.data());
        std::cout.rdbuf(old_out);
        std::cerr.rdbuf(old_err);

        check_true(exit_code == 0, "CLI output-presets list should succeed");
        check_true(out.str().find("youtube_1080p_30") != std::string::npos, "CLI output-presets list should print known preset ids");
    }

    {
        std::vector<std::string> args = {"tachyon", "output-presets", "info", "youtube_4k"};
        std::vector<char*> argv;
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }

        std::ostringstream out;
        std::ostringstream err;
        auto* old_out = std::cout.rdbuf(out.rdbuf());
        auto* old_err = std::cerr.rdbuf(err.rdbuf());
        const int exit_code = tachyon::run_cli(static_cast<int>(argv.size()), argv.data());
        std::cout.rdbuf(old_out);
        std::cerr.rdbuf(old_err);

        check_true(exit_code == 0, "CLI output-presets info should succeed");
        check_true(out.str().find("youtube_4k") != std::string::npos, "CLI output-presets info should print preset name");
        check_true(out.str().find("h265") != std::string::npos, "CLI output-presets info should print preset codec");
    }

    {
        tachyon::SceneSpec scene;
        scene.schema_version = tachyon::SchemaVersion{1, 0, 0};

        tachyon::CompositionSpec comp;
        comp.id = "main";
        comp.width = 1920;
        comp.height = 1080;
        comp.duration = 2.0;
        comp.fps = 30;

        tachyon::LayerSpec legacy_text;
        legacy_text.id = "title";
        legacy_text.type = tachyon::LayerType::Unknown;
        legacy_text.type_string = "text";
        legacy_text.text_content = "Hello";
        legacy_text.start_time = 0.0;
        legacy_text.out_point = 1.0;
        legacy_text.width = 100;
        legacy_text.height = 50;
        legacy_text.transform.position_x = 300.0;
        legacy_text.transform.position_y = 200.0;
        comp.layers.push_back(legacy_text);

        scene.compositions.push_back(comp);

        tachyon::core::SceneValidator validator;
        const auto result = validator.validate(scene);
        check_true(result.is_valid(), "SceneValidator should accept legacy type_string after normalization");
        check_true(std::any_of(result.issues.begin(), result.issues.end(), [](const auto& issue) {
            return issue.path == "composition.main.layers[0].type_string" && issue.severity == tachyon::core::ValidationIssue::Severity::Warning;
        }), "SceneValidator should warn on legacy type_string usage");
    }

    {
        tachyon::SceneSpec scene;
        scene.schema_version = tachyon::SchemaVersion{1, 0, 0};

        tachyon::CompositionSpec comp;
        comp.id = "main";
        comp.width = 1920;
        comp.height = 1080;
        comp.duration = 2.0;
        comp.fps = 30;

        tachyon::LayerSpec legacy_animation;
        legacy_animation.id = "anim";
        legacy_animation.type = tachyon::LayerType::Text;
        legacy_animation.text_content = "Hello";
        legacy_animation.in_preset = "tachyon.textanim.fade_in";
        legacy_animation.animation_out_preset = "tachyon.textanim.fade_out";
        legacy_animation.start_time = 0.0;
        legacy_animation.out_point = 1.0;
        comp.layers.push_back(legacy_animation);

        scene.compositions.push_back(comp);

        tachyon::core::SceneValidator validator;
        const auto result = validator.validate(scene);
        check_true(result.is_valid(), "SceneValidator should accept legacy animation presets with warnings");
        check_true(std::any_of(result.issues.begin(), result.issues.end(), [](const auto& issue) {
            return issue.path == "composition.main.layers[0].in_preset" && issue.severity == tachyon::core::ValidationIssue::Severity::Warning;
        }), "SceneValidator should warn on legacy in_preset usage");
        check_true(std::any_of(result.issues.begin(), result.issues.end(), [](const auto& issue) {
            return issue.path == "composition.main.layers[0].out_preset" && issue.severity == tachyon::core::ValidationIssue::Severity::Warning;
        }), "SceneValidator should warn on legacy out_preset usage");
    }

    {
        tachyon::SceneSpec scene;
        scene.schema_version = tachyon::SchemaVersion{1, 0, 0};
        scene.assets.push_back(tachyon::AssetSpec{"hero-image", "image", "hero.png", "", std::nullopt});

        tachyon::CompositionSpec comp;
        comp.id = "main";
        comp.width = 1920;
        comp.height = 1080;
        comp.duration = 2.0;
        comp.fps = 30;

        tachyon::LayerSpec image_layer;
        image_layer.id = "hero";
        image_layer.type = tachyon::LayerType::Image;
        image_layer.asset_id = "hero-image";
        image_layer.start_time = 0.0;
        image_layer.out_point = 1.0;
        comp.layers.push_back(image_layer);

        scene.compositions.push_back(comp);

        tachyon::core::SceneValidator validator;
        const auto result = validator.validate(scene);
        check_true(result.is_valid(), "SceneValidator should accept image layers with scene asset references");
    }

    {
        tachyon::SceneSpec scene;
        scene.schema_version = tachyon::SchemaVersion{1, 0, 0};

        tachyon::CompositionSpec comp;
        comp.id = "main";
        comp.width = 1920;
        comp.height = 1080;
        comp.duration = 2.0;
        comp.fps = 30;

        tachyon::LayerSpec missing_image;
        missing_image.id = "missing";
        missing_image.type = tachyon::LayerType::Image;
        missing_image.start_time = 0.0;
        missing_image.out_point = 1.0;
        comp.layers.push_back(missing_image);

        scene.compositions.push_back(comp);

        tachyon::core::SceneValidator validator;
        const auto result = validator.validate(scene);
        check_true(!result.is_valid(), "SceneValidator should reject image layers without asset_id");
        check_true(std::any_of(result.issues.begin(), result.issues.end(), [](const auto& issue) {
            return issue.path == "composition.main.layers[0].asset_id" && issue.severity == tachyon::core::ValidationIssue::Severity::Error;
        }), "SceneValidator should report a missing asset_id for image layers");
    }

    {
        tachyon::SceneSpec scene;
        scene.schema_version = tachyon::SchemaVersion{1, 0, 0};

        tachyon::CompositionSpec comp;
        comp.id = "main";
        comp.width = 1920;
        comp.height = 1080;
        comp.duration = 2.0;
        comp.fps = 30;

        tachyon::LayerSpec text_layer;
        text_layer.id = "title";
        text_layer.type = tachyon::LayerType::Text;
        text_layer.text_content = "Hello";
        text_layer.start_time = 0.0;
        text_layer.out_point = 1.0;
        comp.layers.push_back(text_layer);

        scene.compositions.push_back(comp);

        const auto normalized_scene = tachyon::core::normalize_scene_view(scene);
        check_true(normalized_scene.compositions.size() == 1, "NormalizedSceneView preserves compositions");
        check_true(normalized_scene.compositions[0].layers.size() == 1, "NormalizedSceneView preserves layers");
        check_true(normalized_scene.compositions[0].layers[0].type == tachyon::LayerType::Text, "NormalizedSceneView canonicalizes text layer type");
        check_true(normalized_scene.compositions[0].layers[0].source != nullptr, "NormalizedSceneView retains source pointers");
    }

    return g_failures == 0;
}
