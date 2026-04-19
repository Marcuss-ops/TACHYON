#include "tachyon/timeline/time.h"
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

bool run_timeline_tests() {
    g_failures = 0;

    {
        tachyon::FrameRate frame_rate{30, 1};
        const auto sample = tachyon::timeline::sample_frame(frame_rate, 15);
        check_true(sample.frame_number == 15, "sample_frame should preserve the requested frame number");
        check_true(std::abs(sample.composition_time_seconds - 0.5) < 1e-9, "frame 15 at 30 fps should sample to 0.5 seconds");
        check_true(nearly_equal(tachyon::timeline::frame_to_seconds(frame_rate, 45), 1.5), "frame_to_seconds resolves frame 45 at 30 fps");
    }

    {
        const double local = tachyon::timeline::local_time_from_composition(2.0, 1.25);
        check_true(std::abs(local - 0.75) < 1e-9, "local time should subtract layer start time from composition time");
    }

    {
        using namespace tachyon;
        using namespace tachyon::timeline;

        SceneSpec scene;
        CompositionSpec comp;
        comp.id = "main";
        comp.width = 1280;
        comp.height = 720;
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
        title.start_time = 1.0;
        title.in_point = 1.0;
        title.out_point = 3.0;
        title.opacity_property.keyframes = {{0.0, 0.0}, {1.0, 1.0}};
        title.transform.position_property.keyframes = {{0.0, {0.0f, 0.0f}}, {1.0, {100.0f, 50.0f}}};

        LayerSpec cam;
        cam.id = "cam1";
        cam.type = "camera";
        cam.in_point = 0.0;
        cam.out_point = 10.0;
        cam.transform.position_x = 10.0;
        cam.transform.position_y = 20.0;

        comp.layers = {bg, title, cam};
        scene.compositions.push_back(comp);

        check_true(is_layer_visible_at_time(title, 1.5, comp.duration), "layer visible inside in/out range");
        check_true(!is_layer_visible_at_time(title, 3.5, comp.duration), "layer hidden outside out point");

        const auto evaluated = evaluate_composition_frame(EvaluationRequest{&scene, "main", 45});
        check_true(evaluated.has_value(), "evaluate_composition_frame returns a value");
        if (evaluated.has_value()) {
            check_true(nearly_equal(evaluated->time_seconds, 1.5), "evaluated composition time matches frame number");
            check_true(evaluated->layers.size() == 3, "composition with 3 layers produces 3 evaluated states");
            check_true(evaluated->layers[1].visible, "title layer visible at frame 45");
            check_true(nearly_equal(evaluated->layers[1].opacity, 0.5), "linear keyframe opacity resolves correctly");
            check_true(nearly_equal(evaluated->layers[1].transform2.position.x, 50.0), "linear keyframe x resolves correctly");
            check_true(nearly_equal(evaluated->layers[1].transform2.position.y, 25.0), "linear keyframe y resolves correctly");
            check_true(evaluated->camera.enabled, "camera evaluata correttamente");
            check_true(nearly_equal(evaluated->camera.position.x, 10.0), "camera x position resolved");
            check_true(nearly_equal(evaluated->camera.position.y, 20.0), "camera y position resolved");
        }
    }

    return g_failures == 0;
}
