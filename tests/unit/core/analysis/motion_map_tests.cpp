#include "tachyon/core/analysis/motion_map.h"

#include <algorithm>
#include <iostream>
#include <string_view>

namespace {

bool contains(const std::vector<std::string>& values, std::string_view needle) {
    return std::any_of(values.begin(), values.end(), [&](const auto& value) {
        return value.find(needle) != std::string::npos;
    });
}

} // namespace

bool run_motion_map_tests() {
    using namespace tachyon;
    using namespace tachyon::analysis;

    SceneSpec scene;
    CompositionSpec comp;
    comp.id = "main";
    comp.duration = 6.0;
    comp.frame_rate = FrameRate{24, 1};

    LayerSpec layer;
    layer.id = "title";
    layer.type = LayerType::Text;
    layer.text_content = "Hello";
    layer.start_time = 0.0;
    layer.duration = 3.0;
    layer.transition_in.kind = TransitionKind::Fade;
    layer.transition_out.kind = TransitionKind::Dissolve;
    layer.in_preset = "tachyon.textanim.fade_in";
    layer.out_preset = "tachyon.textanim.fade_out";
    TextAnimatorSpec animator;
    animator.name = "text_pop";
    animator.properties.opacity_value = 1.0;
    layer.text_animators.push_back(animator);
    layer.motion_blur = true;
    layer.loop = true;
    comp.layers.push_back(layer);
    scene.compositions.push_back(comp);

    const auto static_report = build_motion_map(scene);
    if (!static_report.compositions.empty() && !static_report.compositions.front().runtime_samples.empty()) {
        std::cerr << "motion_map: runtime samples should be opt-in\n";
        return false;
    }

    MotionMapOptions options;
    options.runtime_samples = 3;

    const auto report = build_motion_map(scene, options);
    if (report.compositions.size() != 1) {
        std::cerr << "motion_map: expected one composition\n";
        return false;
    }

    const auto& summary = report.compositions.front();
    if (summary.id != "main" || summary.duration != 6.0 || summary.fps != 24.0) {
        std::cerr << "motion_map: incorrect composition summary\n";
        return false;
    }

    if (summary.layers.size() != 1) {
        std::cerr << "motion_map: expected one layer summary\n";
        return false;
    }

    if (summary.runtime_samples.size() != 3) {
        std::cerr << "motion_map: expected runtime samples\n";
        return false;
    }

    if (summary.runtime_samples.front().frame_number != 0) {
        std::cerr << "motion_map: expected first runtime sample at frame 0\n";
        return false;
    }

    const auto& layer_summary = summary.layers.front();
    if (!contains(layer_summary.animations, "transition_in:fade")
        || !contains(layer_summary.animations, "transition_out:dissolve")
        || !contains(layer_summary.animations, "text_animators:1")
        || !contains(layer_summary.animations, "motion_blur")
        || !contains(layer_summary.animations, "loop")) {
        std::cerr << "motion_map: expected motion hints\n";
        return false;
    }

    return true;
}
