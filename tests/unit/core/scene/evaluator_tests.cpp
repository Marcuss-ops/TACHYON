#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"

#include <filesystem>
#include <fstream>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

bool nearly_equal(double a, double b) {
    return std::abs(a - b) < 1e-6;
}

const std::filesystem::path& tests_root() {
    static const std::filesystem::path root = TACHYON_TESTS_SOURCE_DIR;
    return root;
}

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::in | std::ios::binary);
    if (!input.is_open()) {
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

} // namespace

bool run_scene_evaluator_tests() {
    g_failures = 0;

    {
        const auto path = tests_root() / "fixtures" / "scenes" / "scene_graph_minimal.json";
        const auto text = read_text_file(path);
        check_true(!text.empty(), "scene graph fixture should be readable");

        const auto parsed = tachyon::parse_scene_spec_json(text);
        check_true(parsed.value.has_value(), "scene graph fixture should parse");
        if (parsed.value.has_value()) {
            const auto validation = tachyon::validate_scene_spec(*parsed.value);
            check_true(validation.ok(), "scene graph fixture should validate");

            const auto evaluated = tachyon::scene::evaluate_composition_state(parsed.value->compositions.front(), std::int64_t(30));
            check_true(evaluated.layers.size() == 2, "scene graph should preserve layer order");
            if (evaluated.layers.size() == 2) {
                check_true(evaluated.layers[0].id == "parent", "parent should stay first in stack order");
                check_true(evaluated.layers[1].id == "child", "child should stay second in stack order");
                check_true(evaluated.layers[0].layer_index == 0, "parent layer index should be 0");
                check_true(evaluated.layers[1].layer_index == 1, "child layer index should be 1");
                
                check_true(nearly_equal(evaluated.layers[0].opacity, 0.5), "parent capacity should match its opacity");
                check_true(nearly_equal(evaluated.layers[1].opacity, 0.75), "child local opacity should match its opacity");
                
                const auto child_wp = evaluated.layers[1].world_position3;
                check_true(nearly_equal(child_wp.x, 120.0), "child world x should include parent transform");
                check_true(nearly_equal(child_wp.y, 60.0), "child world y should include parent transform");
                
                check_true(evaluated.layers[0].visible, "parent should be visible");
                check_true(evaluated.layers[1].visible, "child should be visible");
            }
        }
    }

    {
        tachyon::LayerSpec layer;
        layer.id = "title";
        layer.type = "text";
        layer.name = "Title";
        layer.start_time = 1.0;
        layer.in_point = 0.0;
        layer.out_point = 10.0;
        layer.opacity_property.keyframes = {
            {0.0, 0.0},
            {2.0, 1.0}
        };
        layer.transform.position_property.keyframes = {
            {0.0, {0.0f, 0.0f}},
            {2.0, {100.0f, 50.0f}}
        };
        layer.transform.rotation_property.keyframes = {
            {0.0, 0.0},
            {2.0, 90.0}
        };
        layer.transform.scale_property.value = tachyon::math::Vector2{1.0f, 1.0f};
        layer.opacity_property.keyframes = {
            {0.0, 0.0, tachyon::animation::EasingPreset::EaseIn},
            {2.0, 1.0}
        };

        tachyon::CompositionSpec composition;
        composition.id = "main";
        composition.name = "Main";
        composition.width = 1920;
        composition.height = 1080;
        composition.duration = 10.0;
        composition.frame_rate = {30, 1};
        composition.layers.push_back(layer);

        const auto evaluated = tachyon::scene::evaluate_composition_state(composition, std::int64_t(60));
        check_true(nearly_equal(evaluated.composition_time_seconds, 2.0), "frame 60 at 30 fps should evaluate at 2 seconds");
        check_true(evaluated.layers.size() == 1, "composition should evaluate exactly one layer");
        check_true(nearly_equal(evaluated.layers[0].local_time_seconds, 1.0), "layer local time should be composition time minus start time");
        check_true(evaluated.layers[0].opacity < 0.5, "opacity should ease in instead of interpolating linearly");
        check_true(nearly_equal(evaluated.layers[0].local_transform.position.x, 50.0), "position x should interpolate linearly");
        check_true(nearly_equal(evaluated.layers[0].local_transform.position.y, 25.0), "position y should interpolate linearly");
    }

    {
        tachyon::LayerSpec layer;
        layer.id = "solid_01";
        layer.type = "solid";
        layer.name = "Solid";
        layer.start_time = 0.0;
        layer.in_point = 0.0;
        layer.out_point = 10.0;
        layer.opacity = 0.75;
        layer.transform.position_x = 10.0;
        layer.transform.position_y = 20.0;
        layer.transform.rotation = 30.0;
        layer.transform.scale_x = 2.0;
        layer.transform.scale_y = 3.0;

        const auto evaluated = tachyon::scene::evaluate_layer_state(layer, 0, 0.0);
        check_true(nearly_equal(evaluated.opacity, 0.75), "static opacity should evaluate directly");
        check_true(nearly_equal(evaluated.local_transform.position.x, 10.0), "static position x should evaluate directly");
        check_true(nearly_equal(evaluated.local_transform.position.y, 20.0), "static position y should evaluate directly");
        check_true(nearly_equal(evaluated.local_transform.scale.x, 2.0), "static scale x should evaluate directly");
        check_true(nearly_equal(evaluated.local_transform.scale.y, 3.0), "static scale y should evaluate directly");
    }

    {
        tachyon::LayerSpec camera_layer;
        camera_layer.id = "camera_01";
        camera_layer.type = "camera";
        camera_layer.name = "Camera";
        camera_layer.enabled = true;
        camera_layer.is_3d = true;
        camera_layer.start_time = 0.0;
        camera_layer.in_point = 0.0;
        camera_layer.out_point = 10.0;
        camera_layer.transform3d.position_property.value = tachyon::math::Vector3{15.0f, 25.0f, 0.0f};
        camera_layer.transform3d.rotation_property.value = tachyon::math::Vector3{0.0f, 0.0f, 30.0f};
        camera_layer.transform3d.scale_property.value = tachyon::math::Vector3{1.5f, 1.5f, 1.0f};

        tachyon::CompositionSpec composition;
        composition.id = "camera_comp";
        composition.name = "Camera Composition";
        composition.width = 1920;
        composition.height = 1080;
        composition.duration = 10.0;
        composition.frame_rate = {30, 1};
        composition.layers.push_back(camera_layer);

        tachyon::SceneSpec scene;
        scene.project.id = "proj_001";
        scene.project.name = "Evaluator";
        scene.compositions.push_back(composition);

        const auto evaluated = tachyon::scene::evaluate_scene_composition_state(scene, "camera_comp", std::int64_t(30));
        check_true(evaluated.has_value(), "scene evaluator should resolve composition by id");
        if (evaluated.has_value()) {
            check_true(evaluated->camera.available, "camera state should be available when a camera layer is active");
            check_true(evaluated->camera.layer_id == "camera_01", "camera state should point at the active camera layer");
            check_true(nearly_equal(evaluated->camera.position.x, 15.0), "camera position x should evaluate from camera layer");
            check_true(nearly_equal(evaluated->camera.position.y, 25.0), "camera position y should evaluate from camera layer");
            check_true(nearly_equal(evaluated->camera.camera.aspect, 1920.0 / 1080.0), "camera aspect should derive from composition size");
        }
    }

    {
        tachyon::LayerSpec precomp_layer;
        precomp_layer.id = "precomp_01";
        precomp_layer.type = "precomp";
        precomp_layer.name = "Precomp";
        precomp_layer.enabled = true;
        precomp_layer.start_time = 0.0;
        precomp_layer.in_point = 0.0;
        precomp_layer.out_point = 10.0;
        precomp_layer.precomp_id = std::string{"child_comp"};
        precomp_layer.time_remap_property.value = 1.5;

        tachyon::LayerSpec child_solid;
        child_solid.id = "child_solid";
        child_solid.type = "solid";
        child_solid.name = "Child Solid";
        child_solid.enabled = true;
        child_solid.start_time = 0.0;
        child_solid.in_point = 0.0;
        child_solid.out_point = 10.0;
        child_solid.opacity = 1.0;

        tachyon::CompositionSpec child_comp;
        child_comp.id = "child_comp";
        child_comp.name = "Child";
        child_comp.width = 320;
        child_comp.height = 180;
        child_comp.duration = 10.0;
        child_comp.frame_rate = {30, 1};
        child_comp.layers.push_back(child_solid);

        tachyon::CompositionSpec parent_comp;
        parent_comp.id = "parent_comp";
        parent_comp.name = "Parent";
        parent_comp.width = 320;
        parent_comp.height = 180;
        parent_comp.duration = 10.0;
        parent_comp.frame_rate = {30, 1};
        parent_comp.layers.push_back(precomp_layer);

        tachyon::SceneSpec scene;
        scene.project.id = "proj_remap";
        scene.project.name = "Remap";
        scene.compositions.push_back(child_comp);
        scene.compositions.push_back(parent_comp);

        const auto evaluated = tachyon::scene::evaluate_scene_composition_state(scene, "parent_comp", std::int64_t(0));
        check_true(evaluated.has_value(), "time remap scene should evaluate");
        if (evaluated.has_value()) {
            check_true(nearly_equal(evaluated->layers[0].child_time_seconds, 1.5), "time remap should compute child time from property");
            check_true(evaluated->layers[0].nested_composition != nullptr, "precomp layer should resolve nested composition");
            if (evaluated->layers[0].nested_composition != nullptr) {
                check_true(nearly_equal(evaluated->layers[0].nested_composition->composition_time_seconds, 1.5), "nested composition should inherit remapped child time");
            }
        }
    }

    {
        tachyon::LayerSpec text_layer;
        text_layer.id = "caption";
        text_layer.type = "text";
        text_layer.name = "Caption";
        text_layer.text_content = "Hello {{shot.name}} / {{music.bass}}";

        tachyon::CompositionSpec composition;
        composition.id = "template_comp";
        composition.name = "Template";
        composition.width = 640;
        composition.height = 360;
        composition.duration = 5.0;
        composition.frame_rate = {30, 1};
        composition.layers.push_back(text_layer);

        tachyon::SceneSpec scene;
        scene.project.id = "proj_template";
        scene.project.name = "Template";
        scene.compositions.push_back(composition);

        tachyon::scene::EvaluationVariables vars;
        std::unordered_map<std::string, double> numeric_vars;
        std::unordered_map<std::string, std::string> string_vars;
        numeric_vars["music.bass"] = 0.75;
        string_vars["shot.name"] = "Intro";
        vars.numeric = &numeric_vars;
        vars.strings = &string_vars;

        const auto evaluated = tachyon::scene::evaluate_scene_composition_state(scene, "template_comp", std::int64_t(0), nullptr, vars);
        check_true(evaluated.has_value(), "template scene should evaluate");
        if (evaluated.has_value()) {
            check_true(evaluated->layers.front().text_content == "Hello Intro / 0.750000", "template resolver should expand dotted identifiers");
        }
    }


    return g_failures == 0;
}
