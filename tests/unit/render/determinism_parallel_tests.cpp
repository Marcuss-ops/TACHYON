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

bool run_parallel_determinism_test() {
    std::cout << "[DeterminismParallelTests] Running parallel concurrency determinism test...\n";

    auto scene_spec = scene::Composition("determinism_parallel")
        .size(128, 128)
        .duration(1.0)
        .fps(30)
        .layer("bg", [](scene::LayerBuilder& l) {
            l.solid("bg").color({10, 20, 30, 255});
        })
        .layer("box", [](scene::LayerBuilder& l) {
            l.solid("box").color({100, 200, 50, 255})
                .position(32, 32)
                .opacity(0.8);
        })
        .build_scene();

    SceneCompiler compiler;
    auto compiled_res = compiler.compile(scene_spec);
    if (!compiled_res.diagnostics.ok() || !compiled_res.value.has_value()) return false;
    const auto& compiled = *compiled_res.value;

    RenderJob job = RenderJobBuilder::video_export("determinism_parallel", {0, 4}, "");
    auto plan_res = build_render_plan(scene_spec, job);
    if (!plan_res.diagnostics.ok() || !plan_res.value.has_value()) return false;
    
    auto exec_res = build_render_execution_plan(*plan_res.value, 0);
    if (!exec_res.diagnostics.ok() || !exec_res.value.has_value()) return false;
    const auto& execution_plan = *exec_res.value;

    // Single-threaded budget
    runtime::RenderWorkerBudget single;
    single.frame_concurrency = 1;
    single.pixel_concurrency = 1;
    single.total_threads = 1;

    RenderSession session_a;
    session_a.cache().clear();
    auto result_single = session_a.render(scene_spec, compiled, execution_plan, {}, single);

    // Multi-threaded budget
    runtime::RenderWorkerBudget parallel;
    parallel.frame_concurrency = 4;
    parallel.pixel_concurrency = 1;
    parallel.total_threads = 4;

    RenderSession session_b;
    session_b.cache().clear();
    auto result_parallel = session_b.render(scene_spec, compiled, execution_plan, {}, parallel);

    if (result_single.frames.size() != 5 || result_parallel.frames.size() != 5) {
        std::cerr << "[DeterminismParallelTests] FAIL: Expected exactly 5 frames, got single=" 
                  << result_single.frames.size() << ", parallel=" << result_parallel.frames.size() << "\n";
        return false;
    }

    for (std::size_t i = 0; i < 5; ++i) {
        if (!result_single.frames[i].frame || !result_parallel.frames[i].frame) {
            std::cerr << "[DeterminismParallelTests] FAIL: Frame " << i << " is null\n";
            return false;
        }

        const auto hash_single = hash_frame_rgba8(*result_single.frames[i].frame);
        const auto hash_parallel = hash_frame_rgba8(*result_parallel.frames[i].frame);

        if (hash_single != hash_parallel) {
            std::cerr << "[DeterminismParallelTests] FAIL: Single vs. Parallel hash mismatch on frame " << i << ": "
                      << hash_single << " != " << hash_parallel << "\n";
            return false;
        }
    }

    std::cout << "[DeterminismParallelTests] PASS: parallel determinism confirmed\n";
    return true;
}

}
