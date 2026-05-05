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
        p.id = "tachyon.transition.crossfade";
        p.duration = 0.5;
        p.easing = tachyon::animation::EasingPreset::EaseInOut;
        p.delay = 0.1;
        p.direction = "up";

        const auto spec = build_transition_enter(p);
        check_true(spec.transition_id == "tachyon.transition.crossfade", "Enter copies transition id");
        check_true(spec.type == "tachyon.transition.crossfade", "Enter copies type");
        check_true(std::abs(spec.duration - 0.5) < 1e-9, "Enter copies duration");
        check_true(spec.easing == tachyon::animation::EasingPreset::EaseInOut, "Enter copies easing");
        check_true(std::abs(spec.delay - 0.1) < 1e-9, "Enter copies delay");
        check_true(spec.direction == "up", "Enter copies direction");
    }

    {
        TransitionParams p;
        p.id = "tachyon.transition.slide";
        p.duration = 0.3;

        const auto spec = build_transition_exit(p);
        check_true(spec.transition_id == "tachyon.transition.slide", "Exit copies transition id");
        check_true(spec.type == "tachyon.transition.slide", "Exit copies type");
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
        enter.id = "tachyon.transition.crossfade";
        enter.duration = 0.5;

        TransitionParams exit;
        exit.id = "tachyon.transition.zoom";
        exit.duration = 0.3;

        tachyon::LayerSpec layer;
        apply_transitions(layer, enter, exit);

        check_true(layer.transition_in.transition_id == "tachyon.transition.crossfade", "Apply transitions copies enter id");
        check_true(layer.transition_in.type == "tachyon.transition.crossfade", "Apply transitions copies enter type");
        check_true(layer.transition_out.transition_id == "tachyon.transition.zoom", "Apply transitions copies exit id");
        check_true(layer.transition_out.type == "tachyon.transition.zoom", "Apply transitions copies exit type");
    }

    {
        TransitionParams enter;
        enter.id = "tachyon.transition.slide";

        tachyon::LayerSpec layer;
        apply_transitions(layer, enter);

        check_true(layer.transition_in.transition_id == "tachyon.transition.slide", "Apply only enter copies id");
        check_true(layer.transition_out.transition_id.empty(), "Apply only enter leaves exit empty");
    }

    {
        TransitionParams p;
        p.id = "custom";
        p.parameters["intensity"] = 0.8;
        p.parameters["size"] = 1.2;

        const auto spec = build_transition_enter(p);
        check_true(spec.transition_id == "custom", "Custom id is preserved");
        // Note: parameters are not yet in LayerTransitionSpec, but we might want them there in the future.
        // For now, we just ensure the basic mapping works.
    }

    return g_failures == 0;
}
