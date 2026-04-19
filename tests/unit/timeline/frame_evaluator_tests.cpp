#include "tachyon/timeline/evaluator.h"

#include <cmath>
#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

bool nearly_equal(double a, double b, double eps = 1e-6) {
    return std::abs(a - b) <= eps;
}

} // namespace

bool run_scene_evaluator_tests() {
    using namespace tachyon;
    using namespace tachyon::timeline;

    g_failures = 0;

    SceneSpec scene;
    CompositionSpec comp;
    comp.id = "main";
    comp.width = 1920;
    comp.height = 1080;
    comp.duration = 10.0;
    comp.frame_rate = FrameRate{30, 1};

    LayerSpec bg;
    bg.id = "bg";
    bg.type = "solid";
    bg.in_point = 0.0;
    bg.out_point = 10.0;
    bg.opacity = 1.0;

    LayerSpec title;
    title.id = "title";
    title.type = "text";
    title.name = "Hello";
    title.in_point = 1.0;
    title.out_point = 3.0;
    title.opacity_property.keyframes.push_back({0.0, 0.0});
    title.opacity_property.keyframes.push_back({1.0, 1.0});
    title.transform.position_property.keyframes.push_back({0.0, {0.0f, 0.0f}});
    title.transform.position_property.keyframes.push_back({1.0, {100.0f, 50.0f}});

    LayerSpec cam;
    cam.id = "cam1";
    cam.type = "camera";
    cam.in_point = 0.0;
    cam.out_point = 10.0;
    cam.transform.position_x = 10.0;
    cam.transform.position_y = 20.0;

    comp.layers = {bg, title, cam};
    scene.compositions.push_back(comp);

        const auto evaluated = evaluate_composition_frame(EvaluationRequest{&scene, "main", 45});
        check_true(evaluated.has_value(), "evaluate_composition_frame returns a state");
        if (!evaluated.has_value()) {
            return false;
        }

        check_true(nearly_equal(evaluated->composition_time_seconds, 1.5), "frame 45 at 30 fps resolves to 1.5 seconds");
        check_true(evaluated->layers.size() == 3, "composition with 3 layers produces 3 evaluated states");

        check_true(evaluated->layers[0].visible, "background layer visible in range");
        check_true(evaluated->layers[1].visible, "title layer visible inside in/out window");
        check_true(nearly_equal(evaluated->layers[1].opacity, 1.0), "linear keyframe opacity resolved at local time");
        check_true(nearly_equal(evaluated->layers[1].local_transform.position.x, 50.0), "linear keyframe position.x resolves halfway");
        check_true(nearly_equal(evaluated->layers[1].local_transform.position.y, 25.0), "linear keyframe position.y resolves halfway");

        check_true(evaluated->camera.available, "camera layer resolves an available camera state");
        check_true(nearly_equal(evaluated->camera.position.x, 10.0), "camera x position evaluated correctly");
        check_true(nearly_equal(evaluated->camera.position.y, 20.0), "camera y position evaluated correctly");
    
    const auto proj = evaluated->camera.camera.get_projection_matrix();
    check_true(nearly_equal(proj[11], -1.0), "camera projection uses perspective clip convention");

    const auto hidden = evaluate_composition_frame(EvaluationRequest{&scene, "main", 120});
    check_true(hidden.has_value(), "evaluate_composition_frame still returns state for later frame");
    if (hidden.has_value()) {
        check_true(!hidden->layers[1].visible, "title layer hidden outside out point");
    }

    return g_failures == 0;
}
