#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
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

tachyon::ScalarKeyframeSpec make_scalar_kf(double t, double v, tachyon::animation::EasingPreset e = tachyon::animation::EasingPreset::None) {
    tachyon::ScalarKeyframeSpec kf;
    kf.time = t;
    kf.value = v;
    kf.easing = e;
    return kf;
}

tachyon::Vector2KeyframeSpec make_vector2_kf(double t, float x, float y, tachyon::animation::EasingPreset e = tachyon::animation::EasingPreset::None) {
    tachyon::Vector2KeyframeSpec kf;
    kf.time = t;
    kf.value = {x, y};
    kf.easing = e;
    return kf;
}

} // namespace

bool run_scene_evaluator_tests() {
    g_failures = 0;
    using namespace tachyon;

    {
        LayerSpec layer;
        layer.identity.id = "title";
        layer.identity.type = LayerType::Text;
        layer.identity.name = "Title";
        layer.playback.timing.start = 1.0;
        layer.playback.timing.duration = 10.0;
        
        layer.transform.opacity_property.keyframes.push_back(make_scalar_kf(0.0, 0.0, animation::EasingPreset::EaseIn));
        layer.transform.opacity_property.keyframes.push_back(make_scalar_kf(2.0, 1.0));
        
        layer.transform.transform.position_property.keyframes.push_back(make_vector2_kf(0.0, 0.0f, 0.0f));
        layer.transform.transform.position_property.keyframes.push_back(make_vector2_kf(2.0, 100.0f, 50.0f));
        
        layer.transform.transform.rotation_property.keyframes.push_back(make_scalar_kf(0.0, 0.0));
        layer.transform.transform.rotation_property.keyframes.push_back(make_scalar_kf(2.0, 90.0));
        
        layer.transform.transform.scale_x = 1.0;
        layer.transform.transform.scale_y = 1.0;

        CompositionSpec composition;
        composition.id = "main";
        composition.name = "Main";
        composition.width = 1920;
        composition.height = 1080;
        composition.duration = 10.0;
        composition.frame_rate = {30, 1};
        composition.layers.push_back(layer);

        const auto evaluated = scene::evaluate_composition_state(composition, std::int64_t(60));
        check_true(nearly_equal(evaluated.composition_time_seconds, 2.0), "frame 60 at 30 fps should evaluate at 2 seconds");
        check_true(evaluated.layers.size() == 1, "composition should evaluate exactly one layer");
        check_true(nearly_equal(evaluated.layers[0].playback.local_time_seconds, 1.0), "layer local time should be composition time minus start time");
        check_true(evaluated.layers[0].transform.opacity < 0.5, "opacity should ease in instead of interpolating linearly");
        check_true(nearly_equal(evaluated.layers[0].transform.local_transform.position.x, 50.0), "position x should interpolate linearly");
        check_true(nearly_equal(evaluated.layers[0].transform.local_transform.position.y, 25.0), "position y should interpolate linearly");
    }

    {
        LayerSpec layer;
        layer.identity.id = "solid_01";
        layer.identity.type = LayerType::Solid;
        layer.identity.name = "Solid";
        layer.playback.timing.start = 0.0;
        layer.playback.timing.duration = 10.0;
        layer.transform.opacity = 0.75;
        layer.transform.transform.position_x = 10.0;
        layer.transform.transform.position_y = 20.0;
        layer.transform.transform.rotation = 30.0;
        layer.transform.transform.scale_x = 200.0;
        layer.transform.transform.scale_y = 300.0;

        const auto evaluated = scene::evaluate_layer_state(layer, 0, 0.0);
        check_true(nearly_equal(evaluated.transform.opacity, 0.75), "static opacity should evaluate directly");
        check_true(nearly_equal(evaluated.transform.local_transform.position.x, 10.0), "static position x should evaluate directly");
        check_true(nearly_equal(evaluated.transform.local_transform.position.y, 20.0), "static position y should evaluate directly");
        check_true(nearly_equal(evaluated.transform.local_transform.scale.x, 2.0), "static scale x should evaluate directly");
        check_true(nearly_equal(evaluated.transform.local_transform.scale.y, 3.0), "static scale y should evaluate directly");
    }

    {
        LayerSpec precomp_layer;
        precomp_layer.identity.id = "precomp_01";
        precomp_layer.identity.type = LayerType::Precomp;
        precomp_layer.identity.name = "Precomp";
        precomp_layer.identity.enabled = true;
        precomp_layer.playback.timing.start = 0.0;
        precomp_layer.playback.timing.duration = 10.0;
        precomp_layer.source.precomp_id = "child_comp";
        precomp_layer.playback.temporal.time_remap_property.value = 1.5;

        LayerSpec child_solid;
        child_solid.identity.id = "child_solid";
        child_solid.identity.type = LayerType::Solid;
        child_solid.identity.name = "Child Solid";
        child_solid.identity.enabled = true;
        child_solid.playback.timing.start = 0.0;
        child_solid.playback.timing.duration = 10.0;
        child_solid.transform.opacity = 1.0;

        CompositionSpec child_comp;
        child_comp.id = "child_comp";
        child_comp.name = "Child";
        child_comp.width = 320;
        child_comp.height = 180;
        child_comp.duration = 10.0;
        child_comp.frame_rate = {30, 1};
        child_comp.layers.push_back(child_solid);

        CompositionSpec parent_comp;
        parent_comp.id = "parent_comp";
        parent_comp.name = "Parent";
        parent_comp.width = 320;
        parent_comp.height = 180;
        parent_comp.duration = 10.0;
        parent_comp.frame_rate = {30, 1};
        parent_comp.layers.push_back(precomp_layer);

        SceneSpec scene;
        scene.project.id = "proj_remap";
        scene.project.name = "Remap";
        scene.compositions.push_back(child_comp);
        scene.compositions.push_back(parent_comp);

        const auto evaluated = scene::evaluate_scene_composition_state(scene, "parent_comp", std::int64_t(0));
        check_true(evaluated.has_value(), "time remap scene should evaluate");
        if (evaluated.has_value()) {
            check_true(nearly_equal(evaluated->layers[0].playback.local_time_seconds, 1.5), "time remap should compute child time from property");
            check_true(evaluated->layers[0].nested_composition != nullptr, "precomp layer should resolve nested composition");
            if (evaluated->layers[0].nested_composition != nullptr) {
                check_true(nearly_equal(evaluated->layers[0].nested_composition->composition_time_seconds, 1.5), "nested composition should inherit remapped child time");
            }
        }
    }

    {
        LayerSpec text_layer;
        text_layer.identity.id = "caption";
        text_layer.identity.type = LayerType::Text;
        text_layer.identity.name = "Caption";
        text_layer.text.content = "Hello {{shot.name}} / {{music.bass}}";

        CompositionSpec composition;
        composition.id = "template_comp";
        composition.name = "Template";
        composition.width = 640;
        composition.height = 360;
        composition.duration = 5.0;
        composition.frame_rate = {30, 1};
        composition.layers.push_back(text_layer);

        SceneSpec scene;
        scene.project.id = "proj_template";
        scene.project.name = "Template";
        scene.compositions.push_back(composition);

        scene::EvaluationVariables vars;
        std::unordered_map<std::string, double> numeric_vars;
        std::unordered_map<std::string, std::string> string_vars;
        numeric_vars["music.bass"] = 0.75;
        string_vars["shot.name"] = "Intro";
        vars.numeric = &numeric_vars;
        vars.strings = &string_vars;

        const auto evaluated = scene::evaluate_scene_composition_state(scene, "template_comp", std::int64_t(0), nullptr, vars);
        check_true(evaluated.has_value(), "template scene should evaluate");
        if (evaluated.has_value()) {
            check_true(evaluated->layers.front().text.content == "Hello Intro / 0.750000", "template resolver should expand dotted identifiers");
        }
    }

    return g_failures == 0;
}
