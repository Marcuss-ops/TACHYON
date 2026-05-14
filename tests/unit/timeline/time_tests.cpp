#include "tachyon/core/scene/evaluation/evaluator.h"
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

tachyon::ScalarKeyframeSpec make_scalar_kf(double t, double v) {
    tachyon::ScalarKeyframeSpec kf;
    kf.time = t;
    kf.value = v;
    return kf;
}

tachyon::Vector2KeyframeSpec make_vector2_kf(double t, float x, float y) {
    tachyon::Vector2KeyframeSpec kf;
    kf.time = t;
    kf.value = {x, y};
    return kf;
}

} // namespace

bool run_timeline_tests() {
    g_failures = 0;
    using namespace tachyon;

    {
        // Manual timing checks
        double start_time = 1.0;
        double comp_time = 2.0;
        double local_time = comp_time - start_time;
        check_true(nearly_equal(local_time, 1.0), "local time should subtract layer start time from composition time");
    }

    {
        SceneSpec scene;
        CompositionSpec comp;
        comp.id = "main";
        comp.width = 1280;
        comp.height = 720;
        comp.duration = 10.0;
        comp.frame_rate = FrameRate{30, 1};

        LayerSpec bg;
        bg.identity.id = "bg";
        bg.identity.type = LayerType::Solid;
        bg.playback.timing.start = 0.0;
        bg.playback.timing.duration = 10.0;
        bg.transform.opacity = 1.0;

        LayerSpec title;
        title.identity.id = "title";
        title.identity.type = LayerType::Text;
        title.identity.name = "Hello";
        title.playback.timing.start = 1.0;
        title.playback.timing.duration = 2.0;
        
        title.transform.opacity_property.keyframes.push_back(make_scalar_kf(0.0, 0.0));
        title.transform.opacity_property.keyframes.push_back(make_scalar_kf(1.0, 1.0));
        
        title.transform.transform.position_property.keyframes.push_back(make_vector2_kf(0.0, 0.0f, 0.0f));
        title.transform.transform.position_property.keyframes.push_back(make_vector2_kf(1.0, 100.0f, 50.0f));

        comp.layers = {bg, title};
        scene.compositions.push_back(comp);

        // Frame 45 at 30 fps = 1.5 seconds.
        // Title starts at 1.0. So local time is 0.5.
        // Keyframes at 0.0 and 1.0. 0.5 should interpolate to 0.5.
        
        auto evaluated = scene::evaluate_scene_composition_state(scene, "main", std::int64_t(45));
        check_true(evaluated.has_value(), "evaluate_scene_composition_state returns a value");
        if (evaluated.has_value()) {
            check_true(nearly_equal(evaluated->composition_time_seconds, 1.5), "evaluated composition time matches frame number");
            check_true(evaluated->layers.size() == 2, "composition with 2 layers produces 2 evaluated states");
            check_true(evaluated->layers[1].identity.active, "title layer active at frame 45");
            check_true(nearly_equal(evaluated->layers[1].transform.opacity, 0.5), "linear keyframe opacity resolves correctly");
            check_true(nearly_equal(evaluated->layers[1].transform.local_transform.position.x, 50.0), "linear keyframe x resolves correctly");
            check_true(nearly_equal(evaluated->layers[1].transform.local_transform.position.y, 25.0), "linear keyframe y resolves correctly");
        }
    }

    return g_failures == 0;
}
