#include "tachyon/presets/transition/transition_builders.h"

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

} // namespace

bool run_transition_builder_tests() {
    using namespace tachyon::presets;

    {
        TransitionParams p;
        p.id = "fade";
        p.duration = 0.5;
        p.easing = tachyon::animation::EasingPreset::EaseInOut;
        p.delay = 0.1;
        p.direction = "up";

        const auto spec = build_transition_enter(p);
        check_true(spec.transition_id == "fade", "Enter copies transition id");
        check_true(spec.type == "fade", "Enter copies type");
        check_true(std::abs(spec.duration - 0.5) < 1e-9, "Enter copies duration");
        check_true(spec.easing == tachyon::animation::EasingPreset::EaseInOut, "Enter copies easing");
        check_true(std::abs(spec.delay - 0.1) < 1e-9, "Enter copies delay");
        check_true(spec.direction == "up", "Enter copies direction");
    }

    {
        TransitionParams p;
        p.id = "slide_left";
        p.duration = 0.3;

        const auto spec = build_transition_exit(p);
        check_true(spec.transition_id == "slide_left", "Exit copies transition id");
        check_true(spec.type == "slide_left", "Exit copies type");
        check_true(std::abs(spec.duration - 0.3) < 1e-9, "Exit copies duration");
    }

    {
        TransitionParams p;
        p.id = "";
        p.duration = 0.4;

        const auto spec = build_transition_enter(p);
        check_true(spec.transition_id.empty(), "Empty id keeps transition id empty");
        check_true(spec.type == "none", "Empty id maps to none type");
    }

    {
        TransitionParams enter;
        enter.id = "fade";
        enter.duration = 0.5;

        TransitionParams exit;
        exit.id = "zoom";
        exit.duration = 0.3;

        tachyon::LayerSpec layer;
        apply_transitions(layer, enter, exit);

        check_true(layer.transition_in.transition_id == "fade", "Apply transitions copies enter id");
        check_true(layer.transition_in.type == "fade", "Apply transitions copies enter type");
        check_true(layer.transition_out.transition_id == "zoom", "Apply transitions copies exit id");
        check_true(layer.transition_out.type == "zoom", "Apply transitions copies exit type");
    }

    {
        TransitionParams enter;
        enter.id = "slide_up";

        tachyon::LayerSpec layer;
        apply_transitions(layer, enter);

        check_true(layer.transition_in.transition_id == "slide_up", "Apply only enter copies id");
        check_true(layer.transition_out.transition_id.empty(), "Apply only enter leaves exit empty");
    }

    return g_failures == 0;
}
