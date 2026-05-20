#include "test_utils.h"
#include "determinism/determinism_utils.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/compiler/scene_compiler.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/scene/builder.h"
#include <iostream>

namespace tachyon::test {

bool run_deterministic_single_frame_test() {
    std::cout << "[DeterminismFrameTests] Running single-frame determinism test...\n";

    auto scene_spec = scene::Composition("determinism_single_frame")
        .size(256, 256)
        .duration(1.0)
        .fps(30)
        .layer("bg", [](scene::LayerBuilder& l) {
            l.solid("bg").color({20, 40, 80, 255});
        })
        .layer("box", [](scene::LayerBuilder& l) {
            l.solid("box").color({255, 80, 30, 255})
                .position(64, 64)
                .opacity(0.75);
        })
        .build_scene();

    SceneCompiler compiler;
    auto compiled_res = compiler.compile(scene_spec);
    if (!compiled_res.diagnostics.ok() || !compiled_res.value.has_value()) return false;
    const auto& compiled = *compiled_res.value;

    RenderJob job = RenderJobBuilder::still_image("determinism_single_frame", 0, "");
    auto plan_res = build_render_plan(scene_spec, job);
    if (!plan_res.diagnostics.ok() || !plan_res.value.has_value()) return false;
    
    auto exec_res = build_render_execution_plan(*plan_res.value, 0);
    if (!exec_res.diagnostics.ok() || !exec_res.value.has_value()) return false;
    const auto& execution_plan = *exec_res.value;

    RenderSession session_a;
    auto result_a = session_a.render(scene_spec, compiled, execution_plan);

    RenderSession session_b;
    auto result_b = session_b.render(scene_spec, compiled, execution_plan);

    if (result_a.frames.size() != 1 || result_b.frames.size() != 1) {
        std::cerr << "[DeterminismFrameTests] FAIL: Expected exactly 1 frame rendered, got A=" 
                  << result_a.frames.size() << ", B=" << result_b.frames.size() << "\n";
        return false;
    }

    if (!result_a.frames[0].frame || !result_b.frames[0].frame) {
        std::cerr << "[DeterminismFrameTests] FAIL: Rendered frame is null\n";
        return false;
    }

    const auto hash_a = hash_frame_rgba8(*result_a.frames[0].frame);
    const auto hash_b = hash_frame_rgba8(*result_b.frames[0].frame);

    if (hash_a != hash_b) {
        std::cerr << "[DeterminismFrameTests] FAIL: single frame hash mismatch: "
                  << hash_a << " != " << hash_b << "\n";
        return false;
    }

    std::cout << "[DeterminismFrameTests] PASS: single frame determinism confirmed (" << hash_a << ")\n";
    return true;
}

}
