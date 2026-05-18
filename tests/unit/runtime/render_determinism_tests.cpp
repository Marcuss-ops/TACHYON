#include "test_utils.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/compiler/scene_compiler.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/scene/builder.h"
#include <iostream>
#include <cassert>

using namespace tachyon;

std::uint64_t calculate_checksum(const renderer2d::Framebuffer& frame) {
    const auto& pixels = frame.pixels();
    std::uint64_t sum = 0;
    for (float p : pixels) {
        union { float f; std::uint32_t u; } u;
        u.f = p;
        sum = sum * 31 + u.u;
    }
    return sum;
}

bool run_determinism_tests() {
    std::cout << "[RenderDeterminismTests] Starting RenderDeterminism unit tests...\n";

    auto scene_spec = scene::Composition("determinism_test")
        .size(100, 100)
        .duration(1.0)
        .fps(30)
        .layer("bg", [](scene::LayerBuilder& l) {
            l.solid("bg").color({255, 0, 0, 255});
        })
        .layer("moving", [](scene::LayerBuilder& l) {
            l.solid("moving").color({0, 255, 0, 255})
                .position(0, 0)
                .opacity(scene::anim::lerp(1.0, 0.0, 1.0));
        })
        .build_scene();

    SceneCompiler compiler;
    auto compiled_res = compiler.compile(scene_spec);
    if (!compiled_res.diagnostics.ok() || !compiled_res.value.has_value()) return false;
    const auto& compiled = *compiled_res.value;

    RenderJob job = RenderJobBuilder::video_export("determinism_test", {0, 10}, "");
    auto plan_res = build_render_plan(scene_spec, job);
    if (!plan_res.diagnostics.ok() || !plan_res.value.has_value()) return false;
    
    auto exec_res = build_render_execution_plan(*plan_res.value, 0);
    if (!exec_res.diagnostics.ok() || !exec_res.value.has_value()) return false;
    
    const auto& execution_plan = *exec_res.value;

    RenderSession session;

    // Test 1: Same input, same output (deterministic)
    {
        session.cache().clear();
        auto result1 = session.render(scene_spec, compiled, execution_plan);
        
        session.cache().clear();
        auto result2 = session.render(scene_spec, compiled, execution_plan);

        if (result1.total_pixel_ops != result2.total_pixel_ops) {
            std::cerr << "[RenderDeterminismTests] FAIL: Pixel ops mismatch between identical runs. "
                      << "R1: " << result1.total_pixel_ops << ", R2: " << result2.total_pixel_ops << "\n";
            return false;
        }

        if (result1.frames.size() != result2.frames.size()) {
            std::cerr << "[RenderDeterminismTests] FAIL: Result frame count mismatch\n";
            return false;
        }

        for (std::size_t i = 0; i < result1.frames.size(); ++i) {
            if (!result1.frames[i].frame || !result2.frames[i].frame) {
                std::cerr << "[RenderDeterminismTests] FAIL: Frame " << i << " is null\n";
                return false;
            }
            if (calculate_checksum(*result1.frames[i].frame) != calculate_checksum(*result2.frames[i].frame)) {
                std::cerr << "[RenderDeterminismTests] FAIL: Frame " << i << " checksum mismatch between identical runs\n";
                return false;
            }
        }
        std::cout << "[RenderDeterminismTests] Run-to-run determinism verified!\n";
    }

    // Test 2: Cache ON vs Cache OFF produces same output
    {
        session.cache().clear();
        auto result_cache_on = session.render(scene_spec, compiled, execution_plan);

        RenderSession session_no_cache;
        session_no_cache.cache().set_budget_bytes(0);
        auto result_cache_off = session_no_cache.render(scene_spec, compiled, execution_plan);

        if (result_cache_on.frames.size() != result_cache_off.frames.size()) {
            std::cerr << "[RenderDeterminismTests] FAIL: Cache ON/OFF frame count mismatch\n";
            return false;
        }

        for (std::size_t i = 0; i < result_cache_on.frames.size(); ++i) {
            if (!result_cache_on.frames[i].frame || !result_cache_off.frames[i].frame) {
                std::cerr << "[RenderDeterminismTests] FAIL: Cache ON/OFF Frame " << i << " is null\n";
                return false;
            }
            if (calculate_checksum(*result_cache_on.frames[i].frame) != calculate_checksum(*result_cache_off.frames[i].frame)) {
                std::cerr << "[RenderDeterminismTests] FAIL: Frame " << i << " checksum mismatch between cache ON and OFF\n";
                return false;
            }
        }
        std::cout << "[RenderDeterminismTests] Cache ON/OFF determinism verified!\n";
    }

    std::cout << "[RenderDeterminismTests] ALL TESTS PASSED!\n";
    return true;
}
