#include "test_utils.h"
#include "determinism/determinism_utils.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/compiler/scene_compiler.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/scene/builder.h"
#include <iostream>
#include <vector>

namespace tachyon::test {

bool run_deterministic_sequence_test() {
    std::cout << "[DeterminismSequenceTests] Running keyframe sequence determinism test...\n";

    AnimatedVector2Spec pos_anim;
    Vector2KeyframeSpec kf0, kf1;
    kf0.time = 0.0;
    kf0.value = {10.0f, 48.0f};
    kf1.time = 1000.0;
    kf1.value = {100.0f, 48.0f};
    pos_anim.keyframes = {kf0, kf1};

    auto scene_spec = scene::Composition("determinism_sequence")
        .size(128, 128)
        .duration(1.0)
        .fps(30)
        .clear(ColorSpec{0, 0, 0, 255})
        .layer("bg", [](scene::LayerBuilder& l) {
            l.solid("bg").color({0, 0, 0, 255});
        })
        .layer("moving_box", [&pos_anim](scene::LayerBuilder& l) {
            l.solid("moving_box")
                .size(32, 32)
                .position(pos_anim)
                .color({255, 255, 255, 255});
        })
        .build_scene();

    SceneCompiler compiler;
    auto compiled_res = compiler.compile(scene_spec);
    if (!compiled_res.diagnostics.ok() || !compiled_res.value.has_value()) return false;
    const auto& compiled = *compiled_res.value;

    RenderJob job = RenderJobBuilder::video_export("determinism_sequence", {0, 9}, "");
    auto plan_res = build_render_plan(scene_spec, job);
    if (!plan_res.diagnostics.ok() || !plan_res.value.has_value()) return false;
    
    auto exec_res = build_render_execution_plan(*plan_res.value, 0);
    if (!exec_res.diagnostics.ok() || !exec_res.value.has_value()) return false;
    const auto& execution_plan = *exec_res.value;

    RenderSession session_a;
    auto result_a = session_a.render(scene_spec, compiled, execution_plan);

    RenderSession session_b;
    auto result_b = session_b.render(scene_spec, compiled, execution_plan);

    if (result_a.frames.size() != 10 || result_b.frames.size() != 10) {
        std::cerr << "[DeterminismSequenceTests] FAIL: Expected exactly 10 frames, got A=" 
                  << result_a.frames.size() << ", B=" << result_b.frames.size() << "\n";
        return false;
    }

    std::vector<std::uint64_t> first_run;
    std::vector<std::uint64_t> second_run;

    for (std::size_t i = 0; i < 10; ++i) {
        if (!result_a.frames[i].frame || !result_b.frames[i].frame) {
            std::cerr << "[DeterminismSequenceTests] FAIL: Frame " << i << " is null\n";
            return false;
        }
        first_run.push_back(hash_frame_rgba8(*result_a.frames[i].frame));
        second_run.push_back(hash_frame_rgba8(*result_b.frames[i].frame));
    }

    if (first_run != second_run) {
        std::cerr << "[DeterminismSequenceTests] FAIL: sequence hash mismatch\n";
        for (std::size_t i = 0; i < 10; ++i) {
            if (first_run[i] != second_run[i]) {
                std::cerr << "  Frame " << i << " differs: " << first_run[i] << " != " << second_run[i] << "\n";
            }
        }
        return false;
    }

    std::cout << "[DeterminismSequenceTests] PASS: keyframe sequence determinism confirmed\n";
    return true;
}

}
