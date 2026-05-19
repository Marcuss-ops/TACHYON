#include "test_utils.h"
#include "determinism/determinism_utils.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/compiler/scene_compiler.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/scene/builder.h"
#include <iostream>

namespace tachyon::test {

bool run_cache_determinism_test() {
    std::cout << "[DeterminismCacheTests] Running cache determinism test...\n";

    auto scene_spec = scene::Composition("determinism_cache")
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

    RenderJob job = RenderJobBuilder::video_export("determinism_cache", {0, 0}, "");
    auto plan_res = build_render_plan(scene_spec, job);
    if (!plan_res.diagnostics.ok() || !plan_res.value.has_value()) return false;
    
    auto exec_res = build_render_execution_plan(*plan_res.value, 0);
    if (!exec_res.diagnostics.ok() || !exec_res.value.has_value()) return false;
    const auto& execution_plan = *exec_res.value;

    // Render 1: Cache starts empty
    RenderSession session;
    session.cache().clear();
    auto result_uncached = session.render(scene_spec, compiled, execution_plan);

    // Render 2: Cache is now warm
    auto result_cached = session.render(scene_spec, compiled, execution_plan);

    if (result_uncached.frames.size() != 1 || result_cached.frames.size() != 1) {
        std::cerr << "[DeterminismCacheTests] FAIL: Expected exactly 1 frame, got uncached=" 
                  << result_uncached.frames.size() << ", cached=" << result_cached.frames.size() << "\n";
        return false;
    }

    if (!result_uncached.frames[0].frame || !result_cached.frames[0].frame) {
        std::cerr << "[DeterminismCacheTests] FAIL: Rendered frame is null\n";
        return false;
    }

    const auto hash_uncached = hash_frame_rgba8(*result_uncached.frames[0].frame);
    const auto hash_cached = hash_frame_rgba8(*result_cached.frames[0].frame);

    if (hash_uncached != hash_cached) {
        std::cerr << "[DeterminismCacheTests] FAIL: Uncached vs. Cached hash mismatch: "
                  << hash_uncached << " != " << hash_cached << "\n";
        return false;
    }

    std::cout << "[DeterminismCacheTests] PASS: cache determinism confirmed (" << hash_uncached << ")\n";
    return true;
}

}
