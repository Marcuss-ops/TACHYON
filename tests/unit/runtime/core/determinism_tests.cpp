#include "test_utils.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/frames/frame_hasher.h"
#include "tachyon/runtime/compiler/scene_compiler.h"
#include "tachyon/runtime/policy/worker_policy.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <iostream>
#include <vector>

namespace tachyon {

namespace {

SceneSpec create_complex_test_scene() {
    SceneSpec scene;
    scene.compositions.push_back({});
    auto& comp = scene.compositions.back();
    comp.id = "main";
    comp.width = 1280;
    comp.height = 720;
    comp.duration = 1.0;

    for (int i = 0; i < 5; ++i) {
        comp.layers.push_back({});
        auto& layer = comp.layers.back();
        layer.id = "L" + std::to_string(i);
        layer.type = LayerType::Solid;
        layer.transform.position_x = 100.0 * i;
        layer.transform.position_y = 50.0 * i;
        layer.opacity = 0.5 + (0.05 * i);
    }
    return scene;
}

bool check_determinism() {
    auto scene = create_complex_test_scene();
    
    SceneCompiler compiler;
    auto compiled_res = compiler.compile(scene);
    if (!compiled_res.value) return false;
    auto compiled = *compiled_res.value;

    RenderExecutionPlan execution_plan;
    execution_plan.render_plan.quality_tier = "production";
    execution_plan.frame_tasks.push_back({0, {}}); // Just test frame 0 for speed

    // 1. Sequential Render (1 worker)
    RenderSession seq_session;
    auto seq_result = seq_session.render(scene, compiled, execution_plan, "", 1);

    // 2. Parallel Render (with different worker counts)
    std::vector<int> worker_counts = {2, 4, 8};
    FrameHasher hasher;

    const std::uint64_t h1 = hasher.hash_pixels(seq_result.frames[0].frame->pixels());

    for (int workers : worker_counts) {
        RenderSession par_session;
        auto par_result = par_session.render(scene, compiled, execution_plan, "", workers);

        if (seq_result.frames.size() != par_result.frames.size()) {
            std::cerr << "Frame count mismatch with " << workers << " workers\n";
            return false;
        }

        const std::uint64_t h2 = hasher.hash_pixels(par_result.frames[0].frame->pixels());

        if (h1 != h2) {
            std::cerr << "Deterministic failure at frame 0 with " << workers << " workers (hash " << h1 << " vs " << h2 << ")\n";
            return false;
        }
    }

    return true;
}

} // namespace

} // namespace tachyon

bool run_determinism_tests() {
    bool ok = true;
    ok &= tachyon::check_determinism();
    return ok;
}
