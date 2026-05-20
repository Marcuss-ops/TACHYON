#include "test_utils.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/resource/surface_pool.h"
#include "tachyon/core/render_telemetry.h"
#include <iostream>
#include <cassert>

using namespace tachyon;

bool run_warmup_bake_tests() {
    std::cout << "[WarmupBakeTests] Starting Warmup and Bake unit tests...\n";

    // Test 1: RenderWarmupOptions warmup behavior
    {
        RenderSession session;
        assert(session.surface_pool() != nullptr);
        assert(session.surface_pool()->available_count() == 0);

        SceneSpec scene;
        CompositionSpec comp;
        comp.id = "test_comp";
        comp.width = 128;
        comp.height = 128;
        comp.duration = 1.0;
        scene.compositions.push_back(comp);

        CompiledScene compiled;
        compiled.scene_hash = 12345;

        RenderExecutionPlan exec_plan;
        exec_plan.render_plan.composition.width = 128;
        exec_plan.render_plan.composition.height = 128;
        exec_plan.render_plan.composition.duration = 1.0;
        
        FrameRenderTask task;
        task.frame_number = 0;
        task.time_seconds = 0.0;
        exec_plan.frame_tasks.push_back(task);

        RenderWarmupOptions opts;
        opts.enabled = true;
        opts.warmup_buffers = 4;
        opts.warmup_frame = 0;

        session.warmup(scene, compiled, exec_plan, opts);

        assert(session.surface_pool()->available_count() == 4);
        std::cout << "[WarmupBakeTests] Warmup buffers pre-allocated successfully!\n";
    }

    // Test 2: Static Bake Proof logic
    {
        RenderSession session;
        session.set_static_bake_proof(true);

        SceneSpec scene;
        CompositionSpec comp;
        comp.id = "static_comp";
        comp.width = 64;
        comp.height = 64;
        comp.duration = 2.0; // Multiple frames
        scene.compositions.push_back(comp);

        CompiledScene compiled;
        compiled.scene_hash = 98765;

        RenderExecutionPlan exec_plan;
        exec_plan.render_plan.composition.width = 64;
        exec_plan.render_plan.composition.height = 64;
        exec_plan.render_plan.composition.duration = 2.0;
        
        for (int i = 0; i < 5; ++i) {
            FrameRenderTask task;
            task.frame_number = i;
            task.time_seconds = i * 0.033;
            exec_plan.frame_tasks.push_back(task);
        }

        // Initialize telemetry
        RenderTelemetry::get().init();

        auto res = session.render(scene, compiled, exec_plan, std::filesystem::path{}, ::tachyon::runtime::RenderWorkerBudget{}, nullptr);

        assert(res.output_error.empty());

        assert(RenderTelemetry::get().static_bake_enabled() == true);
        assert(RenderTelemetry::get().static_bake_misses() == 1);
        assert(RenderTelemetry::get().static_bake_hits() == 4);
        assert(RenderTelemetry::get().static_bake_bytes() > 0);
        assert(RenderTelemetry::get().static_bake_build_ms() >= 0.0);

        std::cout << "[WarmupBakeTests] Static Bake Proof verified successfully (hits=4, misses=1)!\n";
    }

    std::cout << "[WarmupBakeTests] ALL TESTS PASSED!\n";
    return true;
}
